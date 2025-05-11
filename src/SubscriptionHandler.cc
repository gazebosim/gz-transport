/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include "gz/transport/SubscriptionHandler.hh"

namespace gz
{
  namespace transport
  {
    inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
    {
    /////////////////////////////////////////////////
    SubscriptionHandlerBase::SubscriptionHandlerBase(
        const std::string &_pUuid,
        const std::string &_nUuid,
        const SubscribeOptions &_opts)
      : opts(_opts),
        periodNs(0.0),
        hUuid(Uuid().ToString()),
        lastCbTimestamp(std::chrono::seconds{0}),
        pUuid(_pUuid),
        nUuid(_nUuid)
    {
      if (this->opts.Throttled())
        this->periodNs = 1e9 / this->opts.MsgsPerSec();
    }

    /////////////////////////////////////////////////
    std::string SubscriptionHandlerBase::ProcUuid() const
    {
      return this->pUuid;
    }

    /////////////////////////////////////////////////
    std::string SubscriptionHandlerBase::NodeUuid() const
    {
      return this->nUuid;
    }

    /////////////////////////////////////////////////
    std::string SubscriptionHandlerBase::HandlerUuid() const
    {
      return this->hUuid;
    }

    /////////////////////////////////////////////////
    bool SubscriptionHandlerBase::IgnoreLocalMessages() const
    {
      return this->opts.IgnoreLocalMessages();
    }

    /////////////////////////////////////////////////
    bool SubscriptionHandlerBase::UpdateThrottling()
    {
      if (!this->opts.Throttled())
        return true;

      Timestamp now = std::chrono::steady_clock::now();

      // Elapsed time since the last callback execution.
      auto elapsed = now - this->lastCbTimestamp;

      if (std::chrono::duration_cast<std::chrono::nanoseconds>(
            elapsed).count() < this->periodNs)
      {
        return false;
      }

      // Update the last callback execution.
      this->lastCbTimestamp = now;
      return true;
    }

    /////////////////////////////////////////////////
    ISubscriptionHandler::ISubscriptionHandler(
        const std::string &_pUuid,
        const std::string &_nUuid,
        const SubscribeOptions &_opts)
      : SubscriptionHandlerBase(_pUuid, _nUuid, _opts)
    {
      // Do nothing
    }

    /////////////////////////////////////////////////
    class RawSubscriptionHandler::Implementation
    {
      public: explicit Implementation(const std::string &_msgType)
        : msgType(_msgType)
      {
        // Do nothing
      }

      public: std::string msgType;

      public: RawCallback callback;
    };

    /////////////////////////////////////////////////
    RawSubscriptionHandler::RawSubscriptionHandler(
        const std::string &_pUuid,
        const std::string &_nUuid,
        const std::string &_msgType,
        const SubscribeOptions &_opts)
      : SubscriptionHandlerBase(_pUuid, _nUuid, _opts),
        pimpl(new Implementation(_msgType))
    {
      // Do nothing
    }

    /////////////////////////////////////////////////
    std::string RawSubscriptionHandler::TypeName()
    {
      return pimpl->msgType;
    }

    /////////////////////////////////////////////////
    void RawSubscriptionHandler::SetCallback(const RawCallback &_callback)
    {
      pimpl->callback = _callback;
    }

#ifdef HAVE_ZENOH
    /////////////////////////////////////////////////
    void RawSubscriptionHandler::SetCallback(
      const RawCallback &_cb,
      std::shared_ptr<zenoh::Session> _session,
      const std::string &_topic)
    {
      zenoh::KeyExpr keyexpr(_topic);
      MessageInfo msgInfo;
      msgInfo.SetTopic(_topic);
      msgInfo.SetType("google::protobuf::Message");
      auto dataHandler = [this, msgInfo](const zenoh::Sample &_sample)
      {
        auto payload = _sample.get_payload().as_string();
        this->RunRawCallback(payload.c_str(), payload.size(), msgInfo);
      };

      this->zSub = std::make_unique<zenoh::Subscriber<void>>(
        _session->declare_subscriber(
          keyexpr, dataHandler, zenoh::closures::none));

      this->SetCallback(std::move(_cb));
    }
#endif

    /////////////////////////////////////////////////
    bool RawSubscriptionHandler::RunRawCallback(
        const char *_msgData, const size_t _size,
        const MessageInfo &_info)
    {
      // Make sure we have a callback
      if (!this->pimpl->callback)
      {
        std::cerr << "RawSubscriptionHandler::RunRawCallback() "
                  << "error: Callback is NULL" << std::endl;
        return false;
      }

      // Check if we need to throttle
      if (!this->UpdateThrottling())
        return true;

      // Trigger the callback
      this->pimpl->callback(_msgData, _size, _info);
      return true;
    }

    /////////////////////////////////////////////////
    RawSubscriptionHandler::~RawSubscriptionHandler()
    {
      // Do nothing. This is here for pimpl.
    }
    }
  }
}
