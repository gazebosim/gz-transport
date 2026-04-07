/*
 * Copyright (C) 2025 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include "gz/transport/config.hh"
#include "gz/transport/ReqHandler.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
// Unlock get_contiguous_view() for SHM receive path.
#define Z_FEATURE_UNSTABLE_API
#include <zenoh.hxx>
#include "ShmHelpers.hh"
#endif

namespace gz::transport
{
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
  {
  /// \internal
  /// \brief Private data for IReqHandler class.
  class IReqHandlerPrivate
  {
    /// \brief Default constructor.
    public: IReqHandlerPrivate(const std::string &_nUuid)
    : hUuid(Uuid().ToString()),
      nUuid(_nUuid),
      requested(false)
    {
    }

    /// \brief Destructor.
    public: virtual ~IReqHandlerPrivate() = default;

    /// \brief Unique handler's UUID.
    public: std::string hUuid;

    /// \brief Node UUID.
    public: std::string nUuid;

    /// \brief When true, the REQ was already sent and the REP should be on
    /// its way. Used to not resend the same REQ more than one time.
    public: bool requested;
  };

  /////////////////////////////////////////////////
  IReqHandler::IReqHandler(const std::string &_nUuid)
    : dataPtr(new IReqHandlerPrivate(_nUuid)),
      rep(""),
      result(false),
      repAvailable(false)
  {
  }

  /////////////////////////////////////////////////
  IReqHandler::~IReqHandler()
  {
  }

  /////////////////////////////////////////////////
  std::string IReqHandler::HandlerUuid() const
  {
    return this->dataPtr->hUuid;
  }

  /////////////////////////////////////////////////
  std::string IReqHandler::NodeUuid() const
  {
    return this->dataPtr->nUuid;
  }

  /////////////////////////////////////////////////
  bool IReqHandler::Requested() const
  {
    return this->dataPtr->requested;
  }

  /////////////////////////////////////////////////
  void IReqHandler::Requested(const bool _value)
  {
    this->dataPtr->requested = _value;
  }

#ifdef HAVE_ZENOH
  /////////////////////////////////////////////////
  void IReqHandler::CreateZenohGet(
    std::shared_ptr<zenoh::Session> _session,
    const std::string &_service)
  {
    // Heap-allocate the sync state so both onReply and onDone can
    // safely reference it even after CreateZenohGet() returns (onDone
    // fires after query timeout, which may be after the caller has
    // already unblocked via onReply).
    auto syncMutex = std::make_shared<std::mutex>();
    auto syncDone = std::make_shared<bool>(false);
    auto syncCv = std::make_shared<std::condition_variable>();

    auto onReply = [this, syncMutex, syncDone, syncCv](
        const zenoh::Reply &_reply)
    {
      if (_reply.is_ok())
      {
        const auto &sample = _reply.get_ok();
        // SHM-optimized receive: get a direct pointer into the SHM buffer
        // when available, avoiding a fragmented copy within Zenoh.
        auto view = sample.get_payload().get_contiguous_view();
        if (view.has_value())
        {
          this->NotifyResult(
            std::string(reinterpret_cast<const char *>(view->data), view->len),
            true);
        }
        else
          this->NotifyResult(sample.get_payload().as_string(), true);
      }
      else
      {
        std::cerr << "Received an error :"
                  << _reply.get_err().get_payload().as_string() << "\n";
      }
      // Wake up immediately after the first reply rather than waiting for the
      // full query timeout. For one-way or unreachable services the fallback
      // onDone handler below still fires after timeout_ms.
      std::lock_guard lock(*syncMutex);
      *syncDone = true;
      syncCv->notify_all();
    };

    auto onDone = [syncMutex, syncDone, syncCv]()
    {
      std::lock_guard lock(*syncMutex);
      *syncDone = true;
      syncCv->notify_all();
    };

    zenoh::Session::GetOptions options;
    // Use a 5 s timeout so one-way requests and unreachable services do not
    // stall the caller (which holds the NodeShared mutex) for Zenoh's default
    // ~10 s query timeout.
    options.timeout_ms = 5000;
    std::string payload;
    this->Serialize(payload);

    if (!payload.empty())
    {
      // Use SHM for large request payloads to avoid a serialization copy.
      // Uses the process-level service SHM pool shared with RepHandler.
      auto *provider = serviceShmProvider();
      if (auto shmBuf = allocShmBuf(provider, payload.size()))
      {
        memcpy(shmBuf->data(), payload.data(), payload.size());
        options.payload = zenoh::Bytes(std::move(*shmBuf));
      }
      else
        options.payload = payload;
    }

    _session->get(_service, "", onReply, onDone, std::move(options));

    std::unique_lock lock(*syncMutex);
    syncCv->wait(lock, [&syncDone] { return *syncDone; });
  }
#endif
  }
}
