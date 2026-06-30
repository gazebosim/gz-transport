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

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "gz/transport/config.hh"
#include "gz/transport/SubscriptionHandler.hh"
#include "gz/transport/TopicUtils.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>
#include "NodeSharedPrivate.hh"
#endif

namespace gz::transport
{
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
  {


  /// \internal
  /// \brief Private data for SubscriptionHandlerBase class.
  class SubscriptionHandlerBasePrivate
  {
    /// \brief Constructor.
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
    public: virtual ~SubscriptionHandlerBasePrivate()
    {
      this->ZenohShutdown();
    }

    /// \brief Zenoh teardown. Safe to call multiple times.
    /// See ZenohTeardownEntity in NodeSharedPrivate.hh for the
    /// shared pattern (atomic guard + detached undeclare).
    public: void ZenohShutdown()
    {
#ifdef HAVE_ZENOH
      ZenohTeardownEntity(this->zenohIsShutdown,
                          this->zSub, this->zToken);
#endif
    }

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

    /// \brief Atomic guard for ZenohShutdown idempotence.
    public: std::atomic<bool> zenohIsShutdown{false};

    /// \brief Temporarily store the dispatch closure passed from the header.
    public: std::function<void(const std::string&, const std::string&)> zenohDispatch;
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

#ifdef HAVE_ZENOH
  /////////////////////////////////////////////////
  void SubscriptionHandlerBase::SetZenohSubscriberDispatch(
      const std::string &/*_topic*/,
      const std::string &/*_expectedType*/,
      std::function<void(const std::string &payload,
                         const std::string &msgType)> _dispatch)
  {
    this->dataPtr->zenohDispatch = std::move(_dispatch);
  }
#endif

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
    if (!this->dataPtr->zenohDispatch)
    {
      std::cerr << "ISubscriptionHandler::CreateGenericZenohSubscriber: "
                << "no dispatch set. Call SetZenohSubscriberDispatch first.\n";
      return;
    }
    auto onSample =
      [_dispatch = std::move(this->dataPtr->zenohDispatch)](
        const zenoh::Sample &_sample)
    {
      auto attachment = _sample.get_attachment();
      if (!attachment.has_value())
        return;
      auto msgType = attachment->get().as_string();
      _dispatch(_sample.get_payload().as_string(), msgType);
    };

    this->dataPtr->zSub = std::make_unique<zenoh::Subscriber<void>>(
      _session->declare_subscriber(
        _topic, std::move(onSample), zenoh::closures::none));

    std::string token = TopicUtils::CreateLivelinessToken(
      _topic, this->ProcUuid(), this->NodeUuid(), "MS", this->TypeName());

    if (token.empty())
      return;

    this->dataPtr->zToken = std::make_unique<zenoh::LivelinessToken>(
      _session->liveliness_declare_token(token));
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
    const FullyQualifiedTopic &_fullyQualifiedTopic)
  {
    if (!_fullyQualifiedTopic.FullTopic())
    {
      std::cerr << "Fully qualified topic is not valid\n";
      return;
    }
    zenoh::KeyExpr keyexpr(*_fullyQualifiedTopic.FullTopic());

    this->SetCallback(_cb);

    const std::string topicShort = _fullyQualifiedTopic.Topic();
    std::weak_ptr<RawSubscriptionHandler> weakSelf = this->weak_from_this();

    auto onSample = [weakSelf, topicShort](const zenoh::Sample &_sample)
    {
      auto self = weakSelf.lock();
      if (!self)
        return;

      auto attachment = _sample.get_attachment();
      if (!attachment.has_value())
        return;
      auto msgType = attachment->get().as_string();

      if (self->TypeName() != kGenericMessageType &&
          self->TypeName() != msgType)
      {
        return;
      }
      if (!self->UpdateThrottling())
        return;

      MessageInfo msgInfo;
      msgInfo.SetTopic(topicShort);
      msgInfo.SetType(msgType);

      auto payload = _sample.get_payload().as_string();
      self->RunRawCallback(payload.data(), payload.size(), msgInfo);
    };

    this->dataPtr->zSub = std::make_unique<zenoh::Subscriber<void>>(
      _session->declare_subscriber(
        keyexpr, std::move(onSample), zenoh::closures::none));

    std::string token = TopicUtils::CreateLivelinessToken(
        *_fullyQualifiedTopic.FullTopic(), this->ProcUuid(), this->NodeUuid(),
        "MS", this->TypeName());

    if (token.empty())
      return;

    this->dataPtr->zToken = std::make_unique<zenoh::LivelinessToken>(
        _session->liveliness_declare_token(token));
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
  }  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport
