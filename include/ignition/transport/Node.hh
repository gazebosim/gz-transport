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
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Node Node.hh
    /// \brief A class that allows a client to communicate with other peers.
    /// There are two main communication modes: pub/sub messages and service
    /// calls.
    class Node
    {
      /// \brief Constructor.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Node(bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new topic.
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _scope Topic scope.
      public: void Advertise(const std::string &_topic,
                             const Scope &_scope = Scope::All);

      /// \brief Unadvertise a topic.
      /// \param[in] _topic Topic name to be unadvertised.
      public: void Unadvertise(const std::string &_topic);

      /// \brief Publish a message.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _message protobuf message.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic,
                          const ProtoMsg &_msg);

      /// \brief Subscribe to a topic registering a callback.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function.
      public: template<typename T> void Subscribe(
          const std::string &_topic,
          void(*_cb)(const std::string &, const T &))
      {
        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
            new SubscriptionHandler<T>(this->dataPtr->nUuidStr));

        // Insert the callback into the handler.
        subscrHandlerPtr->SetCallback(_cb);

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->dataPtr->shared->localSubscriptions.AddHandler(
          _topic, this->dataPtr->nUuidStr, subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        if (std::find(this->dataPtr->topicsSubscribed.begin(),
          this->dataPtr->topicsSubscribed.end(), _topic) ==
          this->dataPtr->topicsSubscribed.end())
        {
          this->dataPtr->topicsSubscribed.push_back(_topic);
        }

        // Discover the list of nodes that publish on the topic.
        this->dataPtr->shared->discovery->DiscoverMsg(_topic);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback member function.
      /// \param[in] _obj Instance containing the member function.
      public: template<typename C, typename T> void Subscribe(
          const std::string &_topic,
          void(C::*_cb)(const std::string &, const T &),
          C* _obj)
      {
        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
          new SubscriptionHandler<T>(this->dataPtr->nUuidStr));

        // Insert the callback into the handler by creating a free function.
        subscrHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2));

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->dataPtr->shared->localSubscriptions.AddHandler(
          _topic, this->dataPtr->nUuidStr, subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        if (std::find(this->dataPtr->topicsSubscribed.begin(),
          this->dataPtr->topicsSubscribed.end(), _topic) ==
          this->dataPtr->topicsSubscribed.end())
        {
          this->dataPtr->topicsSubscribed.push_back(_topic);
        }

        // Discover the list of nodes that publish on the topic.
        this->dataPtr->shared->discovery->DiscoverMsg(_topic);
      }

      /// \brief Unsubscribe to a topic.
      /// \param[in] _topic Topic name to be unsubscribed.
      public: void Unsubscribe(const std::string &_topic);

      /// \brief Advertise a new service call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic name associated to the service call.
      /// \param[in] _cb Callback to handle the service request.
      /// \param[in] _scope Topic scope.
      public: template<typename T1, typename T2> void Advertise(
        const std::string &_topic,
        void(*_cb)(const std::string &, const T1 &, T2 &, bool &),
        const Scope &_scope = Scope::All)
      {
        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Add the topic to the list of advertised service calls
        if (std::find(this->dataPtr->srvsAdvertised.begin(),
              this->dataPtr->srvsAdvertised.end(), _topic) ==
              this->dataPtr->srvsAdvertised.end())
        {
          this->dataPtr->srvsAdvertised.push_back(_topic);
        }

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<T1, T2>> repHandlerPtr(
          new RepHandler<T1, T2>());

        // Insert the callback into the handler.
        repHandlerPtr->SetCallback(_cb);

        // Store the replier handler. Each replier handler is
        // associated with a topic. When the receiving thread gets new requests,
        // it will recover the replier handler associated to the topic and
        // will invoke the service call.
        this->dataPtr->shared->repliers.AddHandler(
          _topic, this->dataPtr->nUuidStr, repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        this->dataPtr->shared->discovery->AdvertiseSrvCall(_topic,
          this->dataPtr->shared->myReplierAddress, "", this->dataPtr->nUuidStr,
          _scope);
      }

      /// \brief Advertise a new service call.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service call.
      /// \param[in] _cb Callback to handle the service request.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _scope Topic scope.
      public: template<typename C, typename T1, typename T2> void Advertise(
        const std::string &_topic,
        void(C::*_cb)(const std::string &, const T1 &, T2 &, bool &),
        C* _obj,
        const Scope &_scope = Scope::All)
      {
        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Add the topic to the list of advertised service calls
        if (std::find(this->dataPtr->srvsAdvertised.begin(),
              this->dataPtr->srvsAdvertised.end(), _topic) ==
              this->dataPtr->srvsAdvertised.end())
        {
          this->dataPtr->srvsAdvertised.push_back(_topic);
        }

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<T1, T2>> repHandlerPtr(
          new RepHandler<T1, T2>(this->dataPtr->nUuidStr));

        // Insert the callback into the handler.
        repHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));

        // Store the replier handler. Each replier handler is
        // associated with a topic. When the receiving thread gets new requests,
        // it will recover the replier handler associated to the topic and
        // will invoke the service call.
        this->dataPtr->shared->repliers.AddHandler(
          _topic, this->dataPtr->nUuidStr, repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        this->dataPtr->shared->discovery->AdvertiseSrvCall(_topic,
          this->dataPtr->shared->myReplierAddress, "", this->dataPtr->nUuidStr,
          _scope);
      }

      /// \brief Request a new service call using a non-blocking call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives.
      /// \return 0 when success.
      public: template<typename T1, typename T2> void Request(
        const std::string &_topic,
        const T1 &_req,
        void(*_cb)(const std::string &_topic, const T2 &, bool))
      {
        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->dataPtr->shared->repliers.GetHandler(_topic, repHandler))
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(_topic, _req, rep, result);
          _cb(_topic, rep, result);
          return;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->dataPtr->nUuidStr));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(_cb);

        // Store the request handler.
        this->dataPtr->shared->requests.AddHandler(
          _topic, this->dataPtr->nUuidStr, reqHandlerPtr);

        // If the responser's address is known, make the request.
        Addresses_M addresses;
        if (this->dataPtr->shared->discovery->GetTopicAddresses(
          _topic, addresses))
        {
          this->dataPtr->shared->SendPendingRemoteReqs(_topic);
        }
        else
        {
          // Discover the service call responser.
          this->dataPtr->shared->discovery->DiscoverSrvCall(_topic);
        }
      }

      /// \brief Request a new service call using a non-blocking call.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives.
      /// \return 0 when success.
      public: template<typename C, typename T1, typename T2> void Request(
        const std::string &_topic,
        const T1 &_req,
        void(C::*_cb)(const std::string &_topic, const T2 &, bool),
        C* _obj)
      {
        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->dataPtr->shared->repliers.GetHandler(_topic, repHandler))
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(_topic, _req, rep, result);
          _cb(_topic, rep, result);
          return;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->dataPtr->nUuidStr));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));

        // Store the request handler.
        this->dataPtr->shared->requests.AddHandler(
          _topic, this->dataPtr->nUuidStr, reqHandlerPtr);

        // If the responser's address is known, make the request.
        Addresses_M addresses;
        if (this->dataPtr->shared->discovery->GetTopicAddresses(
          _topic, addresses))
        {
          this->dataPtr->shared->SendPendingRemoteReqs(_topic);
        }
        else
        {
          // Discover the service call responser.
          this->dataPtr->shared->discovery->DiscoverSrvCall(_topic);
        }
      }

      /// \brief Request a new service call using a blocking call.
      /// \param[in] _topic Topic requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _timeout The request will timeout after '_timeout' ms.
      /// \param[out] _res Protobuf message containing the response.
      /// \param[out] _result Result of the service call.
      /// \return true when the request was executed or false if the timeout
      /// expired.
      public: template<typename T1, typename T2> bool Request(
        const std::string &_topic,
        const T1 &_req,
        const unsigned int &_timeout,
        T2 &_rep,
        bool &_result)
      {
        std::unique_lock<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->dataPtr->shared->repliers.GetHandler(_topic, repHandler))
        {
          // There is a responser in my process, let's use it.
          repHandler->RunLocalCallback(_topic, _req, _rep, _result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->dataPtr->nUuidStr));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Store the request handler.
        this->dataPtr->shared->requests.AddHandler(
          _topic, this->dataPtr->nUuidStr, reqHandlerPtr);

        // If the responser's address is known, make the request.
        Addresses_M addresses;
        if (this->dataPtr->shared->discovery->GetTopicAddresses(
          _topic, addresses))
        {
          this->dataPtr->shared->SendPendingRemoteReqs(_topic);
        }
        else
        {
          // Discover the service call responser.
          this->dataPtr->shared->discovery->DiscoverSrvCall(_topic);
        }

        auto now = std::chrono::system_clock::now();

        // Wait until the REP is available.
        bool executed = reqHandlerPtr->condition.wait_until
          (lk, now + std::chrono::milliseconds(_timeout),
           [&reqHandlerPtr]
           {
             return reqHandlerPtr->repAvailable;
           });

        if (executed)
        {
          if (reqHandlerPtr->result)
            _rep.ParseFromString(reqHandlerPtr->rep);

          _result = reqHandlerPtr->result;
        }

        lk.unlock();

        return executed;
      }

      /// \brief Unadvertise a service call.
      /// \param[in] _topic Topic name to be unadvertised.
      public: void UnadvertiseSrv(const std::string &_topic);

      /// \brief The transport captures SIGINT and SIGTERM (czmq does) and
      /// the function will return true in that case. All the task threads
      /// will terminate.
      /// \return true if SIGINT or SIGTERM has been captured.
      public: bool Interrupted();

      /// \internal
      /// \brief Pointer to private data.
      protected: NodePrivatePtr dataPtr;
    };
  }
}
#endif
