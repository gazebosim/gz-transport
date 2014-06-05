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

#ifndef __IGN_TRANSPORT_TOPICSINFO_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICSINFO_HH_INCLUDED__

#include <czmq.h>
#include <google/protobuf/message.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class TopicInformation TopicsInfo.hh
    /// \brief Store addressing information.
    class AddressInfo
    {
      /// \brief Constructor.
      public: AddressInfo();

      /// \brief Destructor.
      public: virtual ~AddressInfo();

      /// \brief Add a new address associated to a given topic and node UUID.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr New address.
      /// \param[in] _ctrl New control address.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[in] _scope Topic Scope.
      /// \return true if the new address is added or false if the address
      /// was already stored.
      public: bool AddAddress(const std::string &_topic,
                              const std::string &_addr,
                              const std::string &_ctrl,
                              const std::string &_pUuid,
                              const std::string &_nUuid,
                              const Scope &_scope);

      /// \brief Return if there is any address stored for the given topic.
      /// \param[in] _topic Topic name.
      /// \return True if there is at least one address stored for the topic.
      public: bool HasTopic(const std::string &_topic);

      /// \brief Return if there is any address stored for the given topic and
      /// process UUID.
      /// \param[in] _topic Topic name.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \return True if there is at least one address stored for the topic and
      /// process UUID.
      public: bool HasAddresses(const std::string &_topic,
                                const std::string &_pUuid);

      /// \brief Get the address information for a given topic and node UUID.
      /// \param[in] _topic Topic name.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[out] _info Address information requested.
      /// \return true if an entry is found for the given topic and node UUID.
      public: bool GetAddress(const std::string &_topic,
                              const std::string &_pUuid,
                              const std::string &_nUuid,
                              Address_t &_info);

      /// \brief Get the map of addresses stored for a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _info Map of addresses requested.
      /// \return true if at least there is one address stored.
      public: bool GetAddresses(const std::string &_topic,
                                Addresses_M &_info);

      /// \brief Remove an address associated to a given topic, process and node
      /// \param[in] _topic Topic name
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      public: void DelAddressByNode(const std::string &_topic,
                                    const std::string &_pUuid,
                                    const std::string &_nUuid);

      /// \brief Remove all the addresses associated to a given process.
      public: void DelAddressesByProc(const std::string &_pUuid);

      /// \brief Print all the information for debugging purposes.
      public: void Print();

      // The keys are topics. The values are another map, where the key is
      // the process UUID and the value a vector of Address_t (0MQ address,
      // 0MQ control address, node UUID and topic scope).
      private: std::map<std::string, Addresses_M> data;

    };

    /// \class TopicInfo TopicsInfo.hh
    /// \brief Store all the information known about a topic.
    class TopicInfo
    {
      /// \brief Constructor.
      public: TopicInfo();

      /// \brief Destructor.
      public: virtual ~TopicInfo();

      /// brief Is a service call pending?
      public: bool requested;

      /// brief Callback to handle service calls requested by other nodes.
      public: ReqCallback reqCb;

      /// brief Callback to manage the service call's response requested by me.
      public: RepCallback repCb;

      /// \brief Beacon used to periodically advertise this topic.
      public: zbeacon_t *beacon;

      /// \brief List that stores the pending service call requests. Every
      /// element of the list contains the serialized parameters for each
      /// request.
      public: std::list<std::string> pendingReqs;

      /// \brief Subscribers info for this topic.
      public: SubscriptionInfo_M subscribers;

      /// \brief Stores all the subscribers information for this topic.
      public: ISubscriptionHandler_M subscriptionHandlers;
    };

    /// \class TopicsInfo TopicsInfo.hh
    /// \brief Store all the information known about topics.
    class TopicsInfo
    {
      /// \brief Constructor.
      public: TopicsInfo();

      /// \brief Destructor.
      public: virtual ~TopicsInfo();

      /// \brief Return if there is some information about a topic stored.
      /// \param[in] _topic Topic name.
      /// \return true If there is information about the topic.
      public: bool HasTopic(const std::string &_topic);

      /// \brief Return true if we are subscribed to a node advertising
      /// the topic.
      /// \param[in] _topic Topic name.
      /// \return true if we are subscribed.
      public: bool Subscribed(const std::string &_topic);

      /// \brief Return true if I am requesting the service call associated to
      /// the topic.
      /// \param[in] _topic Topic name.
      /// \return true if the service call associated to the topic is requested.
      public: bool Requested(const std::string &_topic);

      /// \brief Get the beacon used to advertise the topic
      /// \param[in] _topic Topic name.
      /// \param[out] _beacon Beacon used to advertise the topic.
      /// \return true if there is a beacon associated to the topicS.
      public: bool GetBeacon(const std::string &_topic, zbeacon_t **_beacon);

      /// \brief Get the REQ callback associated to a topic subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] A pointer to the REQ function registered for a topic.
      /// \return true if there is a REQ callback registered for the topic.
      public: bool GetReqCallback(const std::string &_topic,
                                  ReqCallback &_cb);

      /// \brief Get the REP callback associated to a topic subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] A pointer to the REP function registered for a topic.
      /// \return true if there is a REP callback registered for the topic.
      public: bool GetRepCallback(const std::string &_topic,
                                  RepCallback &_cb);

      /// \brief Returns if there are any pending requests in the queue.
      /// \param[in] _topic Topic name.
      /// \return true if there is any pending request in the queue.
      public: bool PendingReqs(const std::string &_topic);

      /// \brief Set a new service call request to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _value New value to be assigned to the requested member.
      public: void SetRequested(const std::string &_topic,
                                const bool _value);

      /// \brief Set the beacon used to advertise the topic
      /// \param[in] _topic Topic name.
      /// \param[in] _beacon Beacon used to advertise the topic.
      public: void SetBeacon(const std::string &_topic,
                             zbeacon_t *_beacon);

      /// \brief Set a new REQ callback associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _cb New callback.
      public: void SetReqCallback(const std::string &_topic,
                                  const ReqCallback &_cb);

      /// \brief Set a new REP callback associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _cb New callback.
      public: void SetRepCallback(const std::string &_topic,
                                  const RepCallback &_cb);

      /// \brief Add a new service call request to the queue.
      /// \param[in] _topic Topic name.
      /// \param[in] _data Parameters of the request.
      public: void AddReq(const std::string &_topic,
                          const std::string &_data);

      /// \brief Add a new service call request to the queue.
      /// \param[in] _topic Topic name.
      /// \param[out] _data Parameters of the request.
      /// \return true if a request was removed.
      public: bool DelReq(const std::string &_topic,
                          std::string &_data);

      /// \brief Check if a topic has remote subscribers in this node.
      /// \param[in] _topic Topic name.
      /// \return true if a topic has remote subscribers in this node.
      public: bool HasRemoteSubscribers(const std::string &_topic);

      /// \brief Add a new remote subscriber for a topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _proc Uuid UUID of the subscribed process.
      /// \param[in] _node Uuid UUID of the subscribed node.
      public: void AddRemoteSubscriber(const std::string &_topic,
                                       const std::string &_procUuid,
                                       const std::string &_nodeUuid);

      /// \brief Delete a new remote subscriber for a topic.
      /// If the node UUID is found, only the entry for this node will be
      /// removed. However, if the UUID process is found, all the remote
      /// subscribers within this process will be removed.
      /// \param[in] _topic Topic name.
      /// \param[in] _pUuid Subscriber's process UUID.
      /// \param[in] _nUuid Subscriber's node UUID.
      public: void DelRemoteSubscriberByNode(const std::string &_topic,
                                             const std::string &_pUuid,
                                             const std::string &_nUuid);

      /// \brief Delete all the subscriptions of a given process UUID.
      /// \param[in] _pUuid Subscriber's process UUID.
      public: void DelRemoteSubscriberByProc(const std::string &_pUuid);

      /// \brief Get the subscription handlers for a topic. A subscription
      /// handler stores the callback and types associated to a subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] _handlers Subscription handlers.
      public: void GetSubscriptionHandlers(const std::string &_topic,
                                           ISubscriptionHandler_M &_handlers);

      /// \brief Add a subscription handler to a topic. A subscription handler
      /// stores the callback and types associated to a subscription.
      /// \param[in] _topic Topic name.
      /// \param[in] _nodeUuid Node's unique identifier.
      /// \param[in] _msgPtr Subscription handler.
      public: void AddSubscriptionHandler(const std::string &_topic,
                                          const std::string &_nodeUuid,
                                        const ISubscriptionHandlerPtr &_msgPtr);

      /// \brief Remove a subscription handler. The node's uuid
      /// is used as a key to remove the appropriate subscription handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nodeUuid Node's unique identifier.
      public: void RemoveSubscriptionHandler(const std::string &_topic,
                                             const std::string &_nodeUuid);

      /// \brief Check if a topic has a subscription handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nodeUuid Node's unique identifier.
      /// \return true if the topic has a subscription handler registered.
      public: bool HasSubscriptionHandler(const std::string &_topic,
                                          const std::string &_nodeUuid);

      /// \brief Get a reference to the topics map.
      /// \return Reference to the topic map.
      public: Topics_M& GetTopicsInfo();

      /// \brief Check if the topic exists. If not, the topic is created.
      /// \param[in] _topic Topic name.
      private: void CheckAndCreate(const std::string &_topic);

      // Hash with the topic/topicInfo information for pub/sub.
      private: Topics_M topicsInfo;
    };
  }
}

#endif
