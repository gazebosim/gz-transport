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

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include "gz/transport/config.hh"
#include "gz/transport/NodeShared.hh"
#include "gz/transport/ReqHandler.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>
// NodeSharedPrivate.hh defines ZenohQuerierEntry; the public header
// only forward-declares it.
#include "NodeSharedPrivate.hh"
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
      requested(false),
      timeoutMs(5000),
      nodeShared(nullptr)
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

    /// \brief Timeout in milliseconds for the underlying transport
    /// call. Mirrors the timeout passed to Node::Request.
    public: unsigned int timeoutMs;

    /// \internal
    /// \brief Owning NodeShared. Non-owning pointer; NodeShared
    /// outlives all IReqHandlers because they're held in its
    /// storage. Set by SetNodeShared before CreateZenohGet so the
    /// latter can reach the per-process Querier cache.
    public: class NodeShared *nodeShared;
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

  /////////////////////////////////////////////////
  void IReqHandler::SetTimeoutMs(unsigned int _timeoutMs)
  {
    this->dataPtr->timeoutMs = _timeoutMs;
  }

  /////////////////////////////////////////////////
  unsigned int IReqHandler::TimeoutMs() const
  {
    return this->dataPtr->timeoutMs;
  }

  /////////////////////////////////////////////////
  void IReqHandler::SetNodeShared(class NodeShared *_shared)
  {
    this->dataPtr->nodeShared = _shared;
  }

#ifdef HAVE_ZENOH
  /////////////////////////////////////////////////
  void IReqHandler::CreateZenohGet(
    std::shared_ptr<zenoh::Session> _session,
    const std::string &_service)
  {
    const auto userTimeoutMs = this->TimeoutMs();
    const auto deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(userTimeoutMs);

    // Look up (or declare) a persistent Querier via NodeShared.
    // NodeShared owns the cache and clears it during Shutdown(),
    // which runs before the session is dropped at process exit.
    auto *shared = this->dataPtr->nodeShared;
    if (!shared)
    {
      std::cerr << "gz-transport zenoh: NodeShared not provided to "
                << "IReqHandler before CreateZenohGet (call "
                << "SetNodeShared first); aborting request for ["
                << _service << "].\n";
      return;
    }
    auto entry = shared->GetOrDeclareZenohQuerier(_service);
    if (!entry)
    {
      std::cerr << "gz-transport zenoh: no Querier for [" << _service
                << "]; aborting request.\n";
      return;
    }
    // Snapshot the raw Querier pointer once. ZenohQuerierEntry::Shutdown
    // (which may run concurrently from NodeShared::Shutdown) releases
    // the unique_ptr without destroying the underlying object, so the
    // raw pointer stays valid for the lifetime of this stack frame
    // even if the unique_ptr field is nullified afterward.
    zenoh::Querier *querier = entry->querier.get();
    if (!querier)
    {
      std::cerr << "gz-transport zenoh: Querier already shut down for ["
                << _service << "]; aborting request.\n";
      return;
    }
    // _session unused: the Querier holds its own session reference.
    (void)_session;

    // The persistent Querier carries an always-on interest
    // declaration on _service, so by the time the user's first
    // request lands the routing has typically converged. Combined
    // with the synchronous liveliness drain at NodeShared
    // construction, this is what closes the cold-start race.

    // Heap-allocate the sync state so the Zenoh closures (which we
    // cannot cancel from this side) can outlive this stack frame.
    auto syncMutex = std::make_shared<std::mutex>();
    auto syncDone = std::make_shared<bool>(false);
    auto syncCv = std::make_shared<std::condition_variable>();

    // Capture _service BY VALUE. A reference capture would dangle
    // after CreateZenohGet returns: the Zenoh closure can fire on a
    // worker thread after the user-side wait has timed out, and the
    // error log would then print stack garbage instead of the
    // service name.
    auto onReply = [this, syncMutex, syncDone, syncCv, _service]
      (const zenoh::Reply &_reply)
    {
      if (_reply.is_ok())
      {
        const auto &sample = _reply.get_ok();
        this->NotifyResult(sample.get_payload().as_string(), true);
      }
      else
      {
        std::cerr << "gz-transport zenoh: error reply on [" << _service
                  << "]: "
                  << _reply.get_err().get_payload().as_string() << "\n";
      }

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

    zenoh::Querier::GetOptions getOpts =
      zenoh::Querier::GetOptions::create_default();
    std::string payload;
    this->Serialize(payload);
    if (!payload.empty())
      getOpts.payload = zenoh::Bytes(payload);

    try
    {
      querier->get("", onReply, onDone, std::move(getOpts));
    }
    catch (const zenoh::ZException &e)
    {
      std::cerr << "gz-transport zenoh: querier.get failed for ["
                << _service << "]: " << e.what() << "\n";
      return;
    }

    // Bound the C++-side wait. Use whatever budget remains (with a
    // small grace) so we don't deadlock on a stuck Zenoh closure.
    auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining < std::chrono::milliseconds(0))
      remaining = std::chrono::milliseconds(0);
    remaining += std::chrono::milliseconds(500);

    std::unique_lock lock(*syncMutex);
    syncCv->wait_for(lock, remaining, [syncDone] { return *syncDone; });
  }
#endif
  }
}
