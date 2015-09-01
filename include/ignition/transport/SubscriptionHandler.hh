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

#ifndef __IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_INCLUDED__
#define __IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_INCLUDED__

#ifdef _MSC_VER
# pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

namespace ignition
{
  namespace transport
  {
    /// \class ISubscriptionHandler SubscriptionHandler.hh
    /// ignition/transport/SubscriptionHandler.hh
    /// \brief Interface class used to manage generic protobub messages.
    class IGNITION_VISIBLE ISubscriptionHandler
    {
      /// \brief Constructor.
      /// \param[in] _nUuid UUID of the node registering the handler.
      public: ISubscriptionHandler(const std::string &_nUuid)
        : hUuid(Uuid().ToString()),
          nUuid(_nUuid)
      {
      }

      /// \brief Destructor.
      public: virtual ~ISubscriptionHandler()
      {
      }

      /// \brief Executes the local callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _msg Protobuf message received.
      /// \return True when success, false otherwise.
      public: virtual bool RunLocalCallback(const std::string &_topic,
                                           const transport::ProtoMsg &_msg) = 0;

      /// \brief Executes the callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _data Serialized data received. The data will be used
      /// to compose a specific protobuf message and will be passed to the
      /// callback function.
      /// \return True when success, false otherwise.
      public: virtual bool RunCallback(const std::string &_topic,
                                       const std::string &_data) = 0;

      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      public: std::string NodeUuid() const
      {
        return this->nUuid;
      }

      /// \brief Get the unique UUID of this handler.
      /// \return a string representation of the handler UUID.
      public: std::string HandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Unique handler's UUID.
      protected: std::string hUuid;

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
      public: SubscriptionHandler(const std::string &_nUuid)
        : ISubscriptionHandler(_nUuid)
      {
      }

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \return Pointer to the specific protobuf message.
      public: std::shared_ptr<T> CreateMsg(const std::string &_data) const
      {
        // Instantiate a specific protobuf message
        std::shared_ptr<T> msgPtr(new T());

        // Create the message using some serialized data
        msgPtr->ParseFromString(_data);

        return msgPtr;
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback with the following parameters:
      /// \param[in] _topic Topic name.
      /// \param[in] _msg Protobuf message containing the topic update.
      public: void Callback(const std::function <void(
        const std::string &_topic, const T &_msg)> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited.
      public: bool RunLocalCallback(const std::string &_topic,
                                    const transport::ProtoMsg &_msg)
      {
        // Execute the callback (if existing)
        if (this->cb)
        {
          auto msgPtr = google::protobuf::down_cast<const T*>(&_msg);

          // Remove the partition part from the topic.
          std::string topicName = _topic;
          topicName.erase(0, topicName.find_last_of("@") + 1);

          this->cb(topicName, *msgPtr);
          return true;
        }
        else
        {
          std::cerr << "SubscriptionHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          return false;
        }
      }

      // Documentation inherited.
      public: bool RunCallback(const std::string &_topic,
                               const std::string &_data)
      {
        // Instantiate the specific protobuf message associated to this topic.
        auto msg = this->CreateMsg(_data);

        // Execute the callback (if existing).
        if (this->cb)
        {
          // Remove the partition part from the topic.
          std::string topicName = _topic;
          topicName.erase(0, topicName.find_last_of("@") + 1);

          this->cb(topicName, *msg);
          return true;
        }
        else
        {
          std::cerr << "SubscriptionHandler::RunCallback() error: "
                    << "Callback is NULL" << std::endl;
          return false;
        }
      }

      /// \brief Callback to the function registered for this handler with the
      /// following parameters:
      /// \param[in] _topic Topic name.
      /// \param[in] _msg Protobuf message containing the topic update.
      private: std::function<void(const std::string &_topic, const T &_msg)> cb;
    };
  }
}

#endif
