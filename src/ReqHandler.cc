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
#include <zenoh.hxx>
#endif

namespace gz
{
  namespace transport
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
      if (dataPtr)
        delete dataPtr;
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
      std::mutex m;
      std::condition_variable doneSignal;
      bool done = false;
      auto onReply = [this](const zenoh::Reply &_reply)
      {
        if (_reply.is_ok())
        {
          const auto &sample = _reply.get_ok();

          auto attachment = sample.get_attachment();
          if (!attachment.has_value())
          {
            std::cerr << "IReqHandler::CreateZenohGet(): Unable to find "
                      << "attachment. Ignoring message..." << std::endl;
            return;
          }

          this->NotifyResult(sample.get_payload().as_string(), true);
        }
        else
        {
          std::cout << "Received an error :"
                    << _reply.get_err().get_payload().as_string() << "\n";
        }
      };

      auto onDone = [&m, &done, &doneSignal]()
      {
        std::lock_guard lock(m);
        done = true;
        doneSignal.notify_all();
      };

      zenoh::Session::GetOptions options;
      std::string payload;
      this->Serialize(payload);

      if (!payload.empty())
        options.payload = payload;

      // TODO(caguero): Remove.
      options.timeout_ms = 2000u;
      _session->get(_service, "",
                  onReply, onDone, std::move(options));

      std::unique_lock lock(m);
      doneSignal.wait(lock, [&done] { return done; });
    }
#endif
    }
  }
}
