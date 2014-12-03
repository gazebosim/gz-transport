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

#ifdef _MSC_VER
# pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Node Node.hh ignition/transport/Node.hh
    /// \brief A class that allows a client to communicate with other peers.
    /// There are two main communication modes: pub/sub messages and service
    /// calls.
    class IGNITION_VISIBLE Node
    {
      /// \brief Constructor.
      public: Node();

      /// \brief Constructor.
      /// \param[in] _partition Partition name used by this node.
      /// \param[in] _ns Default namespace used by this node. This might
      /// be a prefix that can be added to each advertise message if required.
      public: Node(const std::string &_partition,
                   const std::string &_ns);

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new topic.
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _scope Topic scope.
      /// \return true if the topic was advertised.
      public: bool Advertise(const std::string &_topic,
                             const Scope &_scope = Scope::All);

      /// \brief Get the list of topics advertised by this node.
      /// \return A vector containing all the topics advertised by this node.
      public: std::vector<std::string> GetAdvertisedTopics();

      /// \brief Unadvertise a topic.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \return true if the topic was unadvertised.
      public: bool Unadvertise(const std::string &_topic);

      /// \brief Publish a message.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _message protobuf message.
      /// \return true when success.
      public: bool Publish(const std::string &_topic,
                           const ProtoMsg &_msg);

      /// \brief Subscribe to a topic registering a callback.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function with the following
      /// parameters:
      ///   \param[in] _topic Topic name.
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename T> bool Subscribe(
          const std::string &_topic,
          void(*_cb)(const std::string &_topic, const T &_msg))
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
            new SubscriptionHandler<T>(this->dataPtr->nUuid));

        // Insert the callback into the handler.
        subscrHandlerPtr->SetCallback(_cb);

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->dataPtr->shared->localSubscriptions.AddHandler(
          fullyQualifiedTopic, this->dataPtr->nUuid, subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        this->dataPtr->topicsSubscribed.insert(fullyQualifiedTopic);

        // Discover the list of nodes that publish on the topic.
        this->dataPtr->shared->discovery->DiscoverMsg(fullyQualifiedTopic);

        return true;
      }

      /// \brief Subscribe to a topic registering a callback.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function with the following
      /// parameters:
      ///   \param[in] _topic Topic name.
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename C, typename T> bool Subscribe(
          const std::string &_topic,
          void(C::*_cb)(const std::string &_topic, const T &_msg),
          C *_obj)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
          new SubscriptionHandler<T>(this->dataPtr->nUuid));

        // Insert the callback into the handler by creating a free function.
        subscrHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2));

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->dataPtr->shared->localSubscriptions.AddHandler(
          fullyQualifiedTopic, this->dataPtr->nUuid, subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        this->dataPtr->topicsSubscribed.insert(fullyQualifiedTopic);

        // Discover the list of nodes that publish on the topic.
        this->dataPtr->shared->discovery->DiscoverMsg(fullyQualifiedTopic);

        return true;
      }

      /// \brief Get the list of topics subscribed by this node and its
      /// publisher's information.
      /// \return A map where the keys are the topic names subscribed. For each
      /// key, the value is an 'Addresses_M' map. In an 'Addresses_M', the keys
      /// are the process uuid of the publisher. For each uuid key, the
      /// value contains the list of {0MQ addr, 0MQ ctrl addr, node UUID, scope}
      /// advertising the topic.
      public: std::map<std::string, Addresses_M> GetSubscribedTopics();

      /// \brief Unsubscribe from a topic.
      /// \param[in] _topic Topic name to be unsubscribed.
      /// \return true when successfully unsubscribed or false otherwise.
      public: bool Unsubscribe(const std::string &_topic);

      /// \brief Advertise a new service.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _topic Service name to be advertised.
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _scope Topic scope.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      public: template<typename T1, typename T2> bool Advertise(
        const std::string &_topic,
        void(*_cb)(const std::string &_topic, const T1 &_req,
                   T2 &_rep, bool &_result),
        const Scope &_scope = Scope::All)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Add the topic to the list of advertised services.
        this->dataPtr->srvsAdvertised.insert(fullyQualifiedTopic);

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
          fullyQualifiedTopic, this->dataPtr->nUuid, repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        this->dataPtr->shared->discovery->AdvertiseSrv(fullyQualifiedTopic,
          this->dataPtr->shared->myReplierAddress,
          this->dataPtr->shared->replierId.ToString(), this->dataPtr->nUuid,
          _scope);

        return true;
      }

      /// \brief Advertise a new service.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _topic Service name to be advertised.
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _scope Topic scope.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      public: template<typename C, typename T1, typename T2> bool Advertise(
        const std::string &_topic,
        void(C::*_cb)(const std::string &_topic, const T1 &_req,
                      T2 &_rep, bool &_result),
        C *_obj,
        const Scope &_scope = Scope::All)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // Add the topic to the list of advertised services.
        this->dataPtr->srvsAdvertised.insert(fullyQualifiedTopic);

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<T1, T2>> repHandlerPtr(
          new RepHandler<T1, T2>());

        // Insert the callback into the handler.
        repHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));

        // Store the replier handler. Each replier handler is
        // associated with a topic. When the receiving thread gets new requests,
        // it will recover the replier handler associated to the topic and
        // will invoke the service call.
        this->dataPtr->shared->repliers.AddHandler(
          fullyQualifiedTopic, this->dataPtr->nUuid, repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        this->dataPtr->shared->discovery->AdvertiseSrv(fullyQualifiedTopic,
          this->dataPtr->shared->myReplierAddress,
          this->dataPtr->shared->replierId.ToString(), this->dataPtr->nUuid,
          _scope);

        return true;
      }

      /// \brief Get the list of services advertised by this node.
      /// \return A vector containing all services advertised by this node.
      public: std::vector<std::string> GetAdvertisedServices();

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives. The callback has the following parameters:
      ///   \param[in] _topic Service name to be requested.
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename T1, typename T2> bool Request(
        const std::string &_topic,
        const T1 &_req,
        void(*_cb)(const std::string &_topic, const T2 &_rep, bool _result))
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->dataPtr->shared->repliers.GetHandler(fullyQualifiedTopic,
          repHandler))
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(fullyQualifiedTopic, _req, rep, result);
          _cb(fullyQualifiedTopic, rep, result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->dataPtr->nUuid));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(_cb);

        // Store the request handler.
        this->dataPtr->shared->requests.AddHandler(
          fullyQualifiedTopic, this->dataPtr->nUuid, reqHandlerPtr);

        // If the responser's address is known, make the request.
        Addresses_M addresses;
        if (this->dataPtr->shared->discovery->GetSrvAddresses(
          fullyQualifiedTopic, addresses))
        {
          this->dataPtr->shared->SendPendingRemoteReqs(fullyQualifiedTopic);
        }
        else
        {
          // Discover the service responser.
          this->dataPtr->shared->discovery->DiscoverSrv(fullyQualifiedTopic);
        }

        return true;
      }

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a member function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives. The callback has the following parameters:
      ///   \param[in] _topic Service name to be requested.
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when the service call was succesfully requested.
      public: template<typename C, typename T1, typename T2> bool Request(
        const std::string &_topic,
        const T1 &_req,
        void(C::*_cb)(const std::string &_topic, const T2 &_rep, bool _result),
        C *_obj)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->dataPtr->shared->repliers.GetHandler(fullyQualifiedTopic,
          repHandler))
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(fullyQualifiedTopic, _req, rep, result);
          _cb(fullyQualifiedTopic, rep, result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->dataPtr->nUuid));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));

        // Store the request handler.
        this->dataPtr->shared->requests.AddHandler(
          fullyQualifiedTopic, this->dataPtr->nUuid, reqHandlerPtr);

        // If the responser's address is known, make the request.
        Addresses_M addresses;
        if (this->dataPtr->shared->discovery->GetSrvAddresses(
          fullyQualifiedTopic, addresses))
        {
          this->dataPtr->shared->SendPendingRemoteReqs(fullyQualifiedTopic);
        }
        else
        {
          // Discover the service responser.
          this->dataPtr->shared->discovery->DiscoverSrv(fullyQualifiedTopic);
        }

        return true;
      }

      /// \brief Request a new service using a blocking call.
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
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
          this->dataPtr->ns, _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::unique_lock<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->dataPtr->shared->repliers.GetHandler(fullyQualifiedTopic,
          repHandler))
        {
          // There is a responser in my process, let's use it.
          repHandler->RunLocalCallback(fullyQualifiedTopic, _req, _rep,
            _result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->dataPtr->nUuid));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Store the request handler.
        this->dataPtr->shared->requests.AddHandler(
          fullyQualifiedTopic, this->dataPtr->nUuid, reqHandlerPtr);

        // If the responser's address is known, make the request.
        Addresses_M addresses;
        if (this->dataPtr->shared->discovery->GetSrvAddresses(
          fullyQualifiedTopic, addresses))
        {
          this->dataPtr->shared->SendPendingRemoteReqs(fullyQualifiedTopic);
        }
        else
        {
          // Discover the service responser.
          this->dataPtr->shared->discovery->DiscoverSrv(fullyQualifiedTopic);
        }

        // Wait until the REP is available.
        bool executed = reqHandlerPtr->WaitUntil(lk, _timeout);

        if (executed)
        {
          if (reqHandlerPtr->GetResult())
            _rep.ParseFromString(reqHandlerPtr->GetRep());

          _result = reqHandlerPtr->GetResult();
        }

        lk.unlock();

        return executed;
      }

      /// \brief Unadvertise a service.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \return true if the service was successfully unadvertised.
      public: bool UnadvertiseSrv(const std::string &_topic);

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void GetTopicList(std::vector<std::string> &_topics) const;

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void GetServiceList(std::vector<std::string> &_services) const;

      /// \internal
      /// \brief Pointer to private data.
      protected: NodePrivatePtr dataPtr;
    };
  }
}
#endif
