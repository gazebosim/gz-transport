/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_
#define IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <ignition/msgs/Factory.hh>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/MessageInfo.hh"
#include "ignition/transport/SubscribeOptions.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

namespace ignition
{
  namespace transport
  {
    /// \class ISubscriptionHandler SubscriptionHandler.hh
    /// ignition/transport/SubscriptionHandler.hh
    /// \brief Interface class used to manage generic protobub messages.
    class IGNITION_TRANSPORT_VISIBLE ISubscriptionHandler
    {
      /// \brief Constructor.
      /// \param[in] _nUuid UUID of the node registering the handler.
      /// \param[in] _opts Subscription options.
      public: explicit ISubscriptionHandler(const std::string &_nUuid,
        const SubscribeOptions &_opts = SubscribeOptions())
        : hUuid(Uuid().ToString()),
          opts(_opts),
          lastCbTimestamp(std::chrono::seconds{0}),
          periodNs(0.0),
          nUuid(_nUuid)
      {
        if (this->opts.Throttled())
          this->periodNs = 1e9 / this->opts.MsgsPerSec();
      }

      /// \brief Destructor.
      public: virtual ~ISubscriptionHandler()
      {
      }

      /// \brief Executes the local callback registered for this handler.
      /// \param[in] _msg Protobuf message received.
      /// \param[in] _info Message information (e.g.: topic name).
      /// \return True when success, false otherwise.
      public: virtual bool RunLocalCallback(const ProtoMsg &_msg,
                                            const MessageInfo &_info) = 0;

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \param[in] _type The data type.
      /// \return Pointer to the specific protobuf message.
      public: virtual const std::shared_ptr<ProtoMsg> CreateMsg(
        const std::string &_data,
        const std::string &_type) const = 0;

      /// \brief Get the type of the messages from which this subscriber
      /// handler is subscribed.
      /// \return String representation of the message type.
      public: virtual std::string TypeName() = 0;

      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      public: std::string NodeUuid() const
      {
        return this->nUuid;
      }

      /// \brief Get the unique UUID of this handler.
      /// \return A string representation of the handler UUID.
      public: std::string HandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Check if message subscription is throttled. If so, verify
      /// whether the callback should be executed or not.
      /// \return true if the callback should be executed or false otherwise.
      protected: bool UpdateThrottling()
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

      /// \brief Unique handler's UUID.
      protected: std::string hUuid;

      /// \brief Subscribe options.
      protected: SubscribeOptions opts;

      /// \brief Timestamp of the last callback executed.
      protected: Timestamp lastCbTimestamp;

      /// \brief If throttling is enabled, the minimum period for receiving a
      /// message in nanoseconds.
      protected: double periodNs;

      /// \brief Node UUID.
      private: std::string nUuid;
    };

    /// \class SubscriptionHandler SubscriptionHandler.hh
    /// \brief It creates a subscription handler for a specific protobuf
    /// message. 'T' is the Protobuf message type that will be used for this
    /// particular handler.
    template <typename T> class SubscriptionHandler
      : public ISubscriptionHandler
    {
      // Documentation inherited.
      public: explicit SubscriptionHandler(const std::string &_nUuid,
        const SubscribeOptions &_opts = SubscribeOptions())
        : ISubscriptionHandler(_nUuid, _opts)
      {
      }

      // Documentation inherited.
      public: const std::shared_ptr<ProtoMsg> CreateMsg(
        const std::string &_data,
        const std::string &/*_type*/) const
      {
        // Instantiate a specific protobuf message
        auto msgPtr = std::make_shared<T>();

        // Create the message using some serialized data
        if (!msgPtr->ParseFromString(_data))
        {
          std::cerr << "SubscriptionHandler::CreateMsg() error: ParseFromString"
                    << " failed" << std::endl;
        }

        return msgPtr;
      }

      // Documentation inherited.
      public: std::string TypeName()
      {
        return T().GetTypeName();
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback with the following parameters:
      public: void SetCallback(const MsgCallback<T> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited.
      public: bool RunLocalCallback(const ProtoMsg &_msg,
                                    const MessageInfo &_info)
      {
        // No callback stored.
        if (!this->cb)
        {
          std::cerr << "SubscriptionHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          return false;
        }

        // Check the subscription throttling option.
        if (!this->UpdateThrottling())
          return true;

#if GOOGLE_PROTOBUF_VERSION > 2999999
        auto msgPtr = google::protobuf::down_cast<const T*>(&_msg);
#else
        auto msgPtr = google::protobuf::internal::down_cast<const T*>(&_msg);
#endif

        this->cb(*msgPtr, _info);
        return true;
      }

      /// \brief Callback to the function registered for this handler.
      private: MsgCallback<T> cb;
    };

    /// \brief Specialized template when the user prefers a callbacks that
    /// accepts a generic google::protobuf::message instead of a specific type.
    template <> class SubscriptionHandler<ProtoMsg>
      : public ISubscriptionHandler
    {
      // Documentation inherited.
      public: explicit SubscriptionHandler(const std::string &_nUuid,
        const SubscribeOptions &_opts = SubscribeOptions())
        : ISubscriptionHandler(_nUuid, _opts)
      {
      }

      // Documentation inherited.
      public: const std::shared_ptr<ProtoMsg> CreateMsg(
        const std::string &_data,
        const std::string &_type) const
      {
        std::shared_ptr<google::protobuf::Message> msgPtr;

        const google::protobuf::Descriptor *desc =
          google::protobuf::DescriptorPool::generated_pool()
            ->FindMessageTypeByName(_type);

        // First, check if we have the descriptor from the generated proto
        // classes.
        if (desc)
        {
          msgPtr.reset(google::protobuf::MessageFactory::generated_factory()
            ->GetPrototype(desc)->New());
        }
        else
        {
          // Fallback on Ignition Msgs if the message type is not found.
          msgPtr = ignition::msgs::Factory::New(_type);
        }

        if (!msgPtr)
          return nullptr;

        // Create the message using some serialized data
        if (!msgPtr->ParseFromString(_data))
        {
          std::cerr << "CreateMsg() error: ParseFromString failed" << std::endl;
          return nullptr;
        }

        return msgPtr;
      }

      // Documentation inherited.
      public: std::string TypeName()
      {
        return kGenericMessageType;
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback.
      public: void SetCallback(const MsgCallback<ProtoMsg> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited.
      public: bool RunLocalCallback(const ProtoMsg &_msg,
                                    const MessageInfo &_info)
      {
        // No callback stored.
        if (!this->cb)
        {
          std::cerr << "SubscriptionHandler::RunLocalCallback() "
                    << "error: Callback is NULL" << std::endl;
          return false;
        }

        // Check the subscription throttling option.
        if (!this->UpdateThrottling())
          return true;

        this->cb(_msg, _info);
        return true;
      }

      /// \brief Callback to the function registered for this handler.
      private: MsgCallback<ProtoMsg> cb;
    };
  }
}

#endif
