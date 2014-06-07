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

#ifndef __IGN_TRANSPORT_SUBSCRIPTIONSTORAGE_HH_INCLUDED__
#define __IGN_TRANSPORT_SUBSCRIPTIONSTORAGE_HH_INCLUDED__

#include <map>
#include <string>
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class SubscriptionStorage SubscriptionStorage.hh
    /// \brief Class to store the list of topic subscriptions.
    class SubscriptionStorage
    {
      /// \brief Constructor.
      public: SubscriptionStorage();

      /// \brief Destructor.
      public: virtual ~SubscriptionStorage();

      /// \brief Get the subscription handlers for a topic. A subscription
      /// handler stores the callback and types associated to a subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] _handlers Subscription handlers.
      /// \return true if the topic contains at least one subscription.
      public: void GetSubscriptionHandlers(const std::string &_topic,
                                           ISubscriptionHandler_M &_handlers);

      /// \brief Add a subscription handler to a topic. A subscription handler
      /// stores the callback and types associated to a subscription.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \param[in] _msgPtr Subscription handler.
      public: void AddSubscriptionHandler(const std::string &_topic,
                                          const std::string &_nUuid,
                                       const ISubscriptionHandlerPtr &_handler);

      /// \brief Return true if we are subscribed to a node advertising
      /// the topic.
      /// \param[in] _topic Topic name.
      /// \return true if we are subscribed.
      public: bool Subscribed(const std::string &_topic);

      /// \brief Remove a subscription handler. The node's uuid
      /// is used as a key to remove the appropriate subscription handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      public: void RemoveSubscriptionHandler(const std::string &_topic,
                                             const std::string &_nUuid);

      /// \brief Check if a topic has a subscription handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \return true if the topic has a subscription handler registered.
      public: bool HasSubscriptionHandler(const std::string &_topic,
                                          const std::string &_nUuid);

      /// \brief Stores all the subscribers information for each topic.
      private: std::map<std::string, ISubscriptionHandler_M> subscribers;
    };
  }
}

#endif
