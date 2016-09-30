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
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/NodeShared.hh"
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

    /// \brief Block the current thread until a SIGINT or SIGTERM is received.
    /// Note that this function registers a signal handler. Do not use this
    /// function if you want to manage yourself SIGINT/SIGTERM.
    IGNITION_VISIBLE void waitForShutdown();

    /// \class Node Node.hh ignition/transport/Node.hh
    /// \brief A class that allows a client to communicate with other peers.
    /// There are two main communication modes: pub/sub messages and service
    /// calls.
    class IGNITION_VISIBLE Node
    {
      /// \brief Constructor.
      /// \param[in] _options Node options.
      public: Node(const NodeOptions &_options = NodeOptions());

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new topic. If a topic is currently advertised,
      /// you cannot advertise it a second time (regardless of its type).
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _options Advertise options.
      /// \return true if the topic was succesfully advertised.
      /// \sa AdvertiseOptions.
      public: template<typename T> bool Advertise(const std::string &_topic,
                          const AdvertiseOptions &_options = AdvertiseOptions())
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        auto currentTopics = this->TopicsAdvertised();

        if (currentTopics.find(fullyQualifiedTopic) != currentTopics.end())
        {
          std::cerr << "Topic [" << _topic << "] already advertised. You cannot"
                    << " advertise the same topic twice on the same node."
                    << " If you want to advertise the same topic with different"
                    << " types, use separate nodes" << std::endl;
          return false;
        }

        // Add the topic to the list of advertised topics (if it was not before)
        this->TopicsAdvertised().insert(fullyQualifiedTopic);

        // Notify the discovery service to register and advertise my topic.
        MessagePublisher publisher(fullyQualifiedTopic,
          this->Shared()->myAddress,
          this->Shared()->myControlAddress,
          this->Shared()->pUuid, this->NodeUuid(), _options.Scope(),
          T().GetTypeName());

        if (!this->Shared()->discovery->AdvertiseMsg(publisher))
        {
          std::cerr << "Node::Advertise(): Error advertising a topic. "
                    << "Did you forget to start the discovery service?"
                    << std::endl;
          return false;
        }

        return true;
      }

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
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename T> bool Subscribe(
          const std::string &_topic,
          void(*_cb)(const T &_msg))
      {
        std::function<void(const T &)> f = [_cb](const T & _internalMsg)
        {
          (*_cb)(_internalMsg);
        };

        return this->Subscribe<T>(_topic, f);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// In this version the callback is a lamda function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Lambda function with the following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename T> bool Subscribe(
          const std::string &_topic,
          std::function<void(const T &_msg)> &_cb)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<T>> subscrHandlerPtr(
            new SubscriptionHandler<T>(this->NodeUuid()));

        // Insert the callback into the handler.
        subscrHandlerPtr->SetCallback(_cb);

        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->Shared()->localSubscriptions.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), subscrHandlerPtr);

        // Add the topic to the list of subscribed topics (if it was not before)
        this->TopicsSubscribed().insert(fullyQualifiedTopic);

        // Discover the list of nodes that publish on the topic.
        if (!this->Shared()->discovery->DiscoverMsg(fullyQualifiedTopic))
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
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename C, typename T> bool Subscribe(
          const std::string &_topic,
          void(C::*_cb)(const T &_msg),
          C *_obj)
      {
        std::function<void(const T &)> f = [_cb, _obj](const T & _internalMsg)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1);
          cb(_internalMsg);
        };

        return this->Subscribe<T>(_topic, f);
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
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename T1, typename T2> bool Advertise(
        const std::string &_topic,
        void(*_cb)(const T1 &_req, T2 &_rep, bool &_result),
        const AdvertiseOptions &_options = AdvertiseOptions())
      {
        std::function<void(const T1 &, T2 &, bool &)> f =
          [_cb](const T1 &_internalReq, T2 &_internalRep, bool &_internalResult)
        {
          (*_cb)(_internalReq, _internalRep, _internalResult);
        };

        return this->Advertise<T1, T2>(_topic, f, _options);
      }

      /// \brief Advertise a new service.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename T1, typename T2> bool Advertise(
        const std::string &_topic,
        std::function<void(const T1 &_req, T2 &_rep, bool &_result)> &_cb,
        const AdvertiseOptions &_options = AdvertiseOptions())
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<T1, T2>> repHandlerPtr(
          new RepHandler<T1, T2>());

        // Insert the callback into the handler.
        repHandlerPtr->SetCallback(_cb);

        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Add the topic to the list of advertised services.
        this->SrvsAdvertised().insert(fullyQualifiedTopic);

        // Store the replier handler. Each replier handler is
        // associated with a topic. When the receiving thread gets new requests,
        // it will recover the replier handler associated to the topic and
        // will invoke the service call.
        this->Shared()->repliers.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), repHandlerPtr);

        // Notify the discovery service to register and advertise my responser.
        ServicePublisher publisher(fullyQualifiedTopic,
          this->Shared()->myReplierAddress,
          this->Shared()->replierId.ToString(),
          this->Shared()->pUuid, this->NodeUuid(), _options.Scope(),
          T1().GetTypeName(), T2().GetTypeName());

        if (!this->Shared()->discovery->AdvertiseSrv(publisher))
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
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename C, typename T1, typename T2> bool Advertise(
        const std::string &_topic,
        void(C::*_cb)(const T1 &_req, T2 &_rep, bool &_result),
        C *_obj,
        const AdvertiseOptions &_options = AdvertiseOptions())
      {
        std::function<void(const T1 &, T2 &, bool &)> f =
          [_cb, _obj](const T1 &_internalReq,
                      T2 &_internalRep,
                      bool &_internalResult)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3);
          cb(_internalReq, _internalRep, _internalResult);
        };

        return this->Advertise<T1, T2>(_topic, f, _options);
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
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename T1, typename T2> bool Request(
        const std::string &_topic,
        const T1 &_req,
        void(*_cb)(const T2 &_rep, const bool _result))
      {
        std::function<void(const T2 &, const bool)> f =
          [_cb](const T2 &_internalRep, const bool _internalResult)
        {
          (*_cb)(_internalRep, _internalResult);
        };

        return this->Request<T1, T2>(_topic, _req, f);
      }

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _cb Lambda function executed when the response arrives.
      /// The callback has the following parameters:
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename T1, typename T2> bool Request(
        const std::string &_topic,
        const T1 &_req,
        std::function<void(const T2 &_rep, const bool _result)> &_cb)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        bool localResponserFound;
        IRepHandlerPtr repHandler;
        {
          std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);
          localResponserFound = this->Shared()->repliers.FirstHandler(
            fullyQualifiedTopic, T1().GetTypeName(), T2().GetTypeName(),
              repHandler);
        }

        // If the responser is within my process.
        if (localResponserFound)
        {
          // There is a responser in my process, let's use it.
          T2 rep;
          bool result;
          repHandler->RunLocalCallback(_req, rep, result);

          _cb(rep, result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(_cb);

        {
          std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

          // Store the request handler.
          this->Shared()->requests.AddHandler(
            fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

          // If the responser's address is known, make the request.
          SrvAddresses_M addresses;
          if (this->Shared()->discovery->SrvPublishers(
            fullyQualifiedTopic, addresses))
          {
            this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic,
              T1().GetTypeName(), T2().GetTypeName());
          }
          else
          {
            // Discover the service responser.
            if (!this->Shared()->discovery->DiscoverSrv(fullyQualifiedTopic))
            {
              std::cerr << "Node::Request(): Error discovering a service. "
                        << "Did you forget to start the discovery service?"
                        << std::endl;
              return false;
            }
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
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when the service call was succesfully requested.
      public: template<typename C, typename T1, typename T2> bool Request(
        const std::string &_topic,
        const T1 &_req,
        void(C::*_cb)(const T2 &_rep, const bool _result),
        C *_obj)
      {
        std::function<void(const T2 &, const bool)> f =
          [_cb, _obj](const T2 &_internalRep, const bool _internalResult)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1,
            std::placeholders::_2);
          cb(_internalRep, _internalResult);
        };

        return this->Request<T1, T2>(_topic, _req, f);
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
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<T1, T2>> reqHandlerPtr(
          new ReqHandler<T1, T2>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(_req);

        std::unique_lock<std::recursive_mutex> lk(this->Shared()->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->Shared()->repliers.FirstHandler(fullyQualifiedTopic,
          T1().GetTypeName(), T2().GetTypeName(), repHandler))
        {
          // There is a responser in my process, let's use it.
          repHandler->RunLocalCallback(_req, _rep, _result);
          return true;
        }

        // Store the request handler.
        this->Shared()->requests.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

        // If the responser's address is known, make the request.
        SrvAddresses_M addresses;
        if (this->Shared()->discovery->SrvPublishers(
          fullyQualifiedTopic, addresses))
        {
          this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic,
            T1().GetTypeName(), T2().GetTypeName());
        }
        else
        {
          // Discover the service responser.
          if (!this->Shared()->discovery->DiscoverSrv(fullyQualifiedTopic))
          {
            std::cerr << "Node::Request(): Error discovering a service. "
                      << "Did you forget to start the discovery service?"
                      << std::endl;
            return false;
          }
        }

        // Wait until the REP is available.
        bool executed = reqHandlerPtr->WaitUntil(lk, _timeout);

        // The request was not executed.
        if (!executed)
          return false;

        // The request was executed but did not succeed.
        if (!reqHandlerPtr->Result())
        {
          _result = false;
          return true;
        }

        // Parse the response.
        if (!_rep.ParseFromString(reqHandlerPtr->Response()))
        {
          std::cerr << "Node::Request(): Error Parsing the response"
                    << std::endl;
          _result = false;
          return true;
        }

        _result = true;
        return true;
      }

      /// \brief Unadvertise a service.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \return true if the service was successfully unadvertised.
      public: bool UnadvertiseSrv(const std::string &_topic);

      /// \brief Get the list of topics currently advertised in the network.
      /// Note that this function can block for some time if the
      /// discovery is in its initialization phase.
      /// The value of the "heartbeatInterval" constant, with a default
      /// value of 1000 ms, sets the maximum blocking time period.
      /// \param[out] _topics List of advertised topics.
      public: void TopicList(std::vector<std::string> &_topics) const;

      /// \brief Get the information about a topic.
      /// \param[in] _topic Name of the topic.
      /// \param[out] _publishers List of publishers on the topic
      /// \return False if unable to get topic info
      public: bool TopicInfo(const std::string &_topic,
                             std::vector<MessagePublisher> &_publishers) const;

      /// \brief Get the list of topics currently advertised in the network.
      /// Note that this function can block for some time if the
      /// discovery is in its initialization phase.
      /// The value of the "heartbeatInterval" constant, with a default
      /// value of 1000ms, sets the maximum blocking time period.
      /// \param[out] _topics List of advertised topics.
      public: void ServiceList(std::vector<std::string> &_services) const;

      /// \brief Get the information about a service.
      /// \param[in] _service Name of the service.
      /// \param[out] _publishers List of publishers on the service.
      /// \return False if unable to get service info.
      public: bool ServiceInfo(const std::string &_service,
                              std::vector<ServicePublisher> &_publishers) const;

      /// \brief Get the partition name used by this node.
      /// \return The partition name.
      private: const std::string &Partition() const;

      /// \brief Get the namespace used in this node.
      /// \return The namespace
      private: const std::string &NameSpace() const;

      /// \brief Get a pointer to the shared node (singleton shared by all the
      /// nodes).
      /// \return The pointer to the shared node.
      private: NodeShared *Shared() const;

      /// \brief Get the UUID of this node.
      /// \return The node UUID.
      private: const std::string &NodeUuid() const;

      /// \brief Get the set of topics advertised by this node.
      /// \return The set of advertised topics.
      private: std::unordered_set<std::string> &TopicsAdvertised() const;

      /// \brief Get the set of topics subscribed by this node.
      /// \return The set of subscribed topics.
      private: std::unordered_set<std::string> &TopicsSubscribed() const;

      /// \brief Get the set of services advertised by this node.
      /// \return The set of advertised services.
      private: std::unordered_set<std::string> &SrvsAdvertised() const;

      /// \brief Get the reference to the current node options.
      /// \return Reference to the current node options.
      private: NodeOptions &Options() const;

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::NodePrivate> dataPtr;
    };
  }
}
#endif
