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

#include "gz/transport/config.hh"
#include "gz/transport/SubscriptionHandler.hh"

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
    /// \brief Private data for SubscriptionHandlerBase class.
    class SubscriptionHandlerBasePrivate
    {
      /// \brief Default constructor.
      public: SubscriptionHandlerBasePrivate(
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

      /// \brief Destructor.
      public: virtual ~SubscriptionHandlerBasePrivate() = default;

      /// \brief Subscribe options.
      public: SubscribeOptions opts;

      /// \brief If throttling is enabled, the minimum period for receiving a
      /// message in nanoseconds.
      public: double periodNs;

      /// \brief Partition name.
      public: std::string partition;

      /// \brief Unique handler's UUID.
      public: std::string hUuid;

      /// \brief Timestamp of the last callback executed.
      public: Timestamp lastCbTimestamp;

      /// \brief Process UUID.
      public: std::string pUuid;

      /// \brief Node UUID.
      public: std::string nUuid;

#ifdef HAVE_ZENOH
      /// \brief The zenoh subscriber handler.
      public: std::unique_ptr<zenoh::Subscriber<void>> zSub;

      /// \brief The liveliness token.
      public: std::unique_ptr<zenoh::LivelinessToken> zToken;
#endif
    };

    /////////////////////////////////////////////////
    SubscriptionHandlerBase::SubscriptionHandlerBase(
        const std::string &_pUuid,
        const std::string &_nUuid,
        const SubscribeOptions &_opts)
      : dataPtr(new SubscriptionHandlerBasePrivate(_pUuid, _nUuid, _opts))
    {
    }

    /////////////////////////////////////////////////
    SubscriptionHandlerBase::~SubscriptionHandlerBase()
    {
      if (dataPtr)
        delete dataPtr;
    }

    /////////////////////////////////////////////////
    std::string SubscriptionHandlerBase::ProcUuid() const
    {
      return this->dataPtr->pUuid;
    }

    /////////////////////////////////////////////////
    std::string SubscriptionHandlerBase::NodeUuid() const
    {
      return this->dataPtr->nUuid;
    }

    /////////////////////////////////////////////////
    std::string SubscriptionHandlerBase::HandlerUuid() const
    {
      return this->dataPtr->hUuid;
    }

    /////////////////////////////////////////////////
    bool SubscriptionHandlerBase::IgnoreLocalMessages() const
    {
      return this->dataPtr->opts.IgnoreLocalMessages();
    }

    /////////////////////////////////////////////////
    bool SubscriptionHandlerBase::UpdateThrottling()
    {
      if (!this->dataPtr->opts.Throttled())
        return true;

      Timestamp now = std::chrono::steady_clock::now();

      // Elapsed time since the last callback execution.
      auto elapsed = now - this->dataPtr->lastCbTimestamp;

      if (std::chrono::duration_cast<std::chrono::nanoseconds>(
            elapsed).count() < this->dataPtr->periodNs)
      {
        return false;
      }

      // Update the last callback execution.
      this->dataPtr->lastCbTimestamp = now;
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

#ifdef HAVE_ZENOH
    /////////////////////////////////////////////////
    void ISubscriptionHandler::CreateGenericZenohSubscriber(
      std::shared_ptr<zenoh::Session> _session,
      const std::string &_topic)
    {
        MessageInfo msgInfo;
        msgInfo.SetTopic(_topic);
        msgInfo.SetType(this->TypeName());
        auto dataHandler = [this, msgInfo](const zenoh::Sample &_sample)
        {
          auto attachment = _sample.get_attachment();
          if (attachment.has_value())
          {
            auto output = this->CreateMsg(
              _sample.get_payload().as_string(), attachment->get().as_string());
            this->RunLocalCallback(*output, msgInfo);
          }
          else
          {
            std::cerr << "SubscriptionHandler::SetCallback(): Unable to find "
                      << "attachment. Ignoring message..." << std::endl;
          }
        };

        this->dataPtr->zSub = std::make_unique<zenoh::Subscriber<void>>(
          _session->declare_subscriber(
            _topic, dataHandler, zenoh::closures::none));

        this->dataPtr->zToken = std::make_unique<zenoh::LivelinessToken>(
          _session->liveliness_declare_token(
            "gz" +
            _topic + "@" +
            this->ProcUuid() + "@" +
            this->NodeUuid() + "@" +
            "sub@" +
            this->TypeName()));
    }
#endif

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

      this->dataPtr->zSub = std::make_unique<zenoh::Subscriber<void>>(
        _session->declare_subscriber(
          keyexpr, dataHandler, zenoh::closures::none));

      this->dataPtr->zToken = std::make_unique<zenoh::LivelinessToken>(
          _session->liveliness_declare_token(
            "gz" +
            _topic + "@" +
            this->ProcUuid() + "@" +
            this->NodeUuid() + "@" +
            "sub@" +
            this->TypeName()));

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
