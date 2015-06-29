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
#include <unordered_set>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    class NodePrivate;

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
      /// \return true if the topic was succesfully advertised.
      public: bool Advertise(const std::string &_topic,
                             const Scope_t &_scope = Scope_t::All);

      /// \brief Get the list of topics advertised by this node.
      /// \return A vector containing all the topics advertised by this node.
      public: std::vector<std::string> AdvertisedTopics() const;

      /// \brief Unadvertise a topic.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \return true if the topic was unadvertised.
      public: bool Unadvertise(const std::string &_topic);

      /// \brief Publish a message.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _msg protobuf message.
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
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> discLk(
          this->Shared()->discovery->Mutex());
        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
            new SubscriptionHandler<T>(this->NodeUuid()));

        // Insert the callback into the handler.
        subscrHandlerPtr->Callback(_cb);

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->Shared()->localSubscriptions.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        this->TopicsSubscribed().insert(fullyQualifiedTopic);

        // Discover the list of nodes that publish on the topic.
        if (!this->Shared()->discovery->Discover(fullyQualifiedTopic))
        {
          std::cerr << "Node::Subscribe(): Error discovering a topic. "
                    << "Did you forget to start the discovery service?"
                    << std::endl;
          return false;
        }

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
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> discLk(
          this->Shared()->discovery->Mutex());
        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
          new SubscriptionHandler<T>(this->NodeUuid()));

        // Insert the callback into the handler by creating a free function.
        subscrHandlerPtr->Callback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2));

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->Shared()->localSubscriptions.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        this->TopicsSubscribed().insert(fullyQualifiedTopic);

        // Discover the list of nodes that publish on the topic.
        if (!this->Shared()->discovery->Discover(fullyQualifiedTopic))
        {
          std::cerr << "Node::Subscribe(): Error discovering a topic. "
                    << "Did you forget to start the discovery service?"
                    << std::endl;
          return false;
        }

        return true;
      }

      /// \brief Get the list of topics subscribed by this node. Note that
      /// we might be interested in one topic but we still don't know the
      /// address of a publisher.
      /// \return A vector containing the subscribed topics (even if we do not
      /// have an address for a particular topic yet).
      public: std::vector<std::string> SubscribedTopics() const;

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
        const Scope_t &_scope = Scope_t::All)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic, true))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> discLk(
          this->Shared()->discovery->Mutex());
        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Add the topic to the list of advertised services.
        this->SrvsAdvertised().insert(fullyQualifiedTopic);

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<T1, T2>> repHandlerPtr(
          new RepHandler<T1, T2>());

        // Insert the callback into the handler.
        repHandlerPtr->Callback(_cb);

        // Store the replier handler. Each replier handler is
        // associated with a topic. When the receiving thread gets new requests,
        // it will recover the replier handler associated to the topic and
        // will invoke the service call.
        this->Shared()->repliers.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        std::shared_ptr<Publisher> publisher =
        std::make_shared<ServicePublisher>(fullyQualifiedTopic,
          this->Shared()->myReplierAddress,
          this->Shared()->replierId.ToString(),
          this->Shared()->pUuid, this->NodeUuid(), _scope, "unused",
          "unused");
        if (!this->Shared()->discovery->Advertise(publisher))
        {
          std::cerr << "Node::Advertise(): Error advertising a service. "
                    << "Did you forget to start the discovery service?"
                    << std::endl;
          return false;
        }

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
        const Scope_t &_scope = Scope_t::All)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic, true))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> discLk(
          this->Shared()->discovery->Mutex());
        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Add the topic to the list of advertised services.
        this->SrvsAdvertised().insert(fullyQualifiedTopic);

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<T1, T2>> repHandlerPtr(
          new RepHandler<T1, T2>());

        // Insert the callback into the handler.
        repHandlerPtr->Callback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));

        // Store the replier handler. Each replier handler is
        // associated with a topic. When the receiving thread gets new requests,
        // it will recover the replier handler associated to the topic and
        // will invoke the service call.
        this->Shared()->repliers.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        std::shared_ptr<Publisher> publisher =
        std::make_shared<ServicePublisher>(fullyQualifiedTopic,
          this->Shared()->myReplierAddress,
          this->Shared()->replierId.ToString(),
          this->Shared()->pUuid, this->NodeUuid(), _scope, "unused",
          "unused");

        if (!this->Shared()->discovery->Advertise(publisher))
        {
          std::cerr << "Node::Advertise(): Error advertising a service. "
                    << "Did you forget to start the discovery service?"
                    << std::endl;
          return false;
        }

        return true;
      }

      /// \brief Get the list of services advertised by this node.
      /// \return A vector containing all services advertised by this node.
      public: std::vector<std::string> AdvertisedServices() const;

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
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic, true))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> discLk(
          this->Shared()->discovery->Mutex());
        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->Shared()->repliers.GetHandler(fullyQualifiedTopic,
          repHandler))
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(fullyQualifiedTopic, _req, rep, result);

          // Notify the requester with the response and remove the partition
          // part from the topic name.
          std::string topicName = fullyQualifiedTopic;
          topicName.erase(0, topicName.find_last_of("@") + 1);

          _cb(topicName, rep, result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->Message(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->Callback(_cb);

        // Store the request handler.
        this->Shared()->requests.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

        // If the responser's address is known, make the request.
        SrvAddresses_M addresses;
        if (this->Shared()->discovery->SrvPublishers(
          fullyQualifiedTopic, addresses))
        {
          this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic);
        }
        else
        {
          // Discover the service responser.
          if (!this->Shared()->discovery->Discover(
            fullyQualifiedTopic))
          {
            std::cerr << "Node::Request(): Error discovering a service. "
                      << "Did you forget to start the discovery service?"
                      << std::endl;
            return false;
          }
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
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic, true))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> discLk(
          this->Shared()->discovery->Mutex());
        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->Shared()->repliers.GetHandler(fullyQualifiedTopic,
          repHandler))
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(fullyQualifiedTopic, _req, rep, result);

          // Notify the requester with the response and remove the partition
          // part from the topic name.
          std::string topicName = fullyQualifiedTopic;
          topicName.erase(0, topicName.find_last_of("@") + 1);

          _cb(topicName, rep, result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));

        // Store the request handler.
        this->Shared()->requests.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

        // If the responser's address is known, make the request.
        SrvAddresses_M addresses;
        if (this->Shared()->discovery->SrvPublishers(
          fullyQualifiedTopic, addresses))
        {
          this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic);
        }
        else
        {
          // Discover the service responser.
          if (!this->Shared()->discovery->Discover(
            fullyQualifiedTopic))
          {
            std::cerr << "Node::Request(): Error discovering a service. "
                      << "Did you forget to start the discovery service?"
                      << std::endl;
            return false;
          }
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
        if (!TopicUtils::GetFullyQualifiedName(this->Partition(),
          this->NameSpace(), _topic, fullyQualifiedTopic, true))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        this->Shared()->discovery->Mutex().lock();
        std::unique_lock<std::recursive_mutex> lk(this->Shared()->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->Shared()->repliers.GetHandler(fullyQualifiedTopic,
          repHandler))
        {
          // There is a responser in my process, let's use it.
          repHandler->RunLocalCallback(fullyQualifiedTopic, _req, _rep,
            _result);
          this->Shared()->discovery->Mutex().unlock();
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->Message(_req);

        // Store the request handler.
        this->Shared()->requests.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

        // If the responser's address is known, make the request.
        SrvAddresses_M addresses;
        if (this->Shared()->discovery->SrvPublishers(
          fullyQualifiedTopic, addresses))
        {
          this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic);
        }
        else
        {
          // Discover the service responser.
          if (!this->Shared()->discovery->Discover(
            fullyQualifiedTopic))
          {
            std::cerr << "Node::Request(): Error discovering a service. "
                      << "Did you forget to start the discovery service?"
                      << std::endl;
            return false;
          }
        }
        this->Shared()->discovery->Mutex().unlock();

        // Wait until the REP is available.
        bool executed = reqHandlerPtr->WaitUntil(lk, _timeout);

        if (executed)
        {
          if (reqHandlerPtr->Result())
            _rep.ParseFromString(reqHandlerPtr->Response());

          _result = reqHandlerPtr->Result();
        }

        lk.unlock();

        return executed;
      }

      /// \brief Unadvertise a service.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \return true if the service was successfully unadvertised.
      // public: bool UnadvertiseSrv(const std::string &_topic);

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void TopicList(std::vector<std::string> &_topics) const;

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void ServiceList(std::vector<std::string> &_services) const;

      /// \brief Get the partition name used by this node.
      /// \return The partition name.
      private: const std::string& Partition() const;

      /// \brief Get the namespace used in this node.
      /// \return The namespace
      private: const std::string& NameSpace() const;

      /// \brief Get a pointer to the shared node (singleton shared by all the
      /// nodes).
      /// \return The pointer to the shared node.
      private: NodeShared* Shared() const;

      /// \brief Get the UUID of this node.
      /// \return The node UUID.
      private: const std::string& NodeUuid() const;

      /// \brief Get the set of topics subscribed by this node.
      /// \return The set of subscribed topics.
      private: std::unordered_set<std::string>& TopicsSubscribed() const;

      /// \brief Get the set of services advertised by this node.
      /// \return The set of advertised services.
      private: std::unordered_set<std::string>& SrvsAdvertised() const;

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::NodePrivate> dataPtr;
    };
  }
}
#endif
