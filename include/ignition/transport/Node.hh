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

#ifndef __IGN_TRANSPORT_NODE_HH_INCLUDED__
#define __IGN_TRANSPORT_NODE_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <uuid/uuid.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Node Node.hh
    /// \brief A transport node to send and receive data using a
    /// publication/subscription paradigm.
    class Node
    {
      /// \brief Constructor.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Node(bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new topic.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _scope Topic scope.
      public: void Advertise(const std::string &_topic,
                             const Scope &_scope = Scope::All);

      /// \brief Unadvertise a topic.
      /// \param[in] _topic Topic to be unadvertised.
      public: void Unadvertise(const std::string &_topic);

      /// \brief Publish a message.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _message protobuf message.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic,
                          const ProtoMsg &_msg);

      /// \brief Subscribe to a topic registering a callback. In this version
      /// the callback is a free function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function.
      public: template<typename T> void Subscribe(
          const std::string &_topic,
          void(*_cb)(const std::string &, const T &))
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
            new SubscriptionHandler<T>(this->nUuidStr));

        // Insert the callback into the handler.
        subscrHandlerPtr->SetCallback(_cb);

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->dataPtr->topics.AddSubscriptionHandler(
          _topic, this->nUuidStr, subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        if (std::find(this->topicsSubscribed.begin(),
          this->topicsSubscribed.end(), _topic) == this->topicsSubscribed.end())
        {
          this->topicsSubscribed.push_back(_topic);
        }

        // Discover the list of nodes that publish on the topic.
        this->dataPtr->discovery->Discover(_topic);
      }

      /// \brief Subscribe to a topic registering a callback. In this version
      /// the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback member function.
      /// \param[in] _obj Instance.
      public: template<typename C, typename T> void Subscribe(
          const std::string &_topic,
          void(C::*_cb)(const std::string &, const T &), C* _obj)
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
            new SubscriptionHandler<T>(this->nUuidStr));

        // Insert the callback into the handler by creating a free function.
        subscrHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2));

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->dataPtr->topics.AddSubscriptionHandler(
          _topic, this->nUuidStr, subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        if (std::find(this->topicsSubscribed.begin(),
          this->topicsSubscribed.end(), _topic) == this->topicsSubscribed.end())
        {
          this->topicsSubscribed.push_back(_topic);
        }

        // Discover the list of nodes that publish on the topic.
        this->dataPtr->discovery->Discover(_topic);
      }

      /// \brief Unsubscribe to a topic.
      /// \param[in] _topic Topic to be unsubscribed.
      public: void Unsubscribe(const std::string &_topic);

      /// \brief The transport captures SIGINT and SIGTERM (czmq does) and
      /// the function will return true in that case. All the task threads
      /// will terminate.
      /// \return true if SIGINT or SIGTERM has been captured.
      public: bool Interrupted();

      /// \internal
      /// \brief Shared pointer to private data.
      protected: NodePrivatePtr dataPtr;

      /// \brief The list of topics subscribed by this node.
      private: std::vector<std::string> topicsSubscribed;

      /// \brief The list of topics advertised by this node.
      private: std::vector<std::string> topicsAdvertised;

      /// \brief Node UUID. This ID is unique for each node.
      private: uuid_t nUuid;

      /// \brief Node UUID in string format.
      private: std::string nUuidStr;
    };
  }
}
#endif
