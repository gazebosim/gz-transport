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
#ifndef IGN_TRANSPORT_NODE_HH_
#define IGN_TRANSPORT_NODE_HH_

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

// ToDo: Remove after fixing the warnings
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <ignition/msgs.hh>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Export.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscribeOptions.hh"
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
    void IGNITION_TRANSPORT_VISIBLE waitForShutdown();

    /// \class Node Node.hh ignition/transport/Node.hh
    /// \brief A class that allows a client to communicate with other peers.
    /// There are two main communication modes: pub/sub messages and service
    /// calls.
    class IGNITION_TRANSPORT_VISIBLE Node
    {
      class PublisherPrivate;

      /// \brief A class that is used to store information about an
      /// advertised publisher. An instance of this class is returned
      /// from Node::Advertise, and should be used in subsequent
      /// Node::Publisher::Publish calls.
      ///
      /// ## Pseudo code example ##
      ///
      ///    auto pub = myNode.Advertise<MsgType>("topic_name");
      ///    if (pub)
      ///    {
      ///      MsgType msg;
      ///
      ///      // Note that this version of Publish will copy the message
      ///      // when publishing to interprocess subscribers.
      ///      pub.Publish(msg);
      ///    }
      public: class IGNITION_TRANSPORT_VISIBLE Publisher
      {
        /// \brief Default constructor.
        public: Publisher();

        /// \brief Constructor.
        /// \param[in] _publisher A message publisher.
        public: explicit Publisher(const MessagePublisher &_publisher);

        /// \brief Destructor.
        public: virtual ~Publisher();

        /// \brief Allows this class to be evaluated as a boolean.
        /// \return True if valid
        /// \sa Valid
        public: operator bool();

        /// \brief Allows this class to be evaluated as a boolean (const).
        /// \return True if valid
        /// \sa Valid
        public: operator bool() const;

        /// \brief Return true if valid information, such as a non-empty
        /// topic name, is present.
        /// \return True if this object can be used in Publish() calls.
        public: bool Valid() const;

        /// \brief Publish a message. This function will copy the message
        /// when publishing to interprocess subscribers. This copy is
        /// necessary to facilitate asynchronous publication.
        /// \param[in] _msg A google::protobuf message.
        /// \return true when success.
        public: bool Publish(const ProtoMsg &_msg);

        /// \brief Publish a raw pre-serialized message.
        ///
        /// \warning This function is only intended for advanced users. The
        /// standard publishing function, Publish(const ProtoMsg &_msg), will
        /// ensure that your message is correctly serialized. It is strongly
        /// recommended that you use the standard publishing function unless
        /// there is a specific reason for using this one (e.g. you are
        /// forwarding or playing back data instead of serializing/deserializing
        /// it). We currently only support the serialization scheme of protobuf.
        ///
        /// \warning This function will copy the message data when
        /// publishing to remote subscribers (interprocess communication).
        ///
        /// \note This function will deserialize the message when sending it to
        /// local (intraprocess) subscribers.
        ///
        /// \param[in] _msgData A std::string that represents a
        /// serialized google::protobuf message.
        /// \param[in] _msgType A std::string that contains the message type
        /// name.
        /// \return true when success.
        public: bool PublishRaw(
          const std::string &_msgData,
          const std::string &_msgType);

        /// \brief Check if message publication is throttled. If so, verify
        /// whether the next message should be published or not.
        /// \return true if the message should be published or false otherwise.
        private: bool UpdateThrottling();

        /// \brief Return true if this publisher has subscribers.
        /// \return True if subscribers have connected to this publisher.
        public: bool HasConnections() const;

        /// \internal
        /// \brief Smart pointer to private data.
        /// This is std::shared_ptr because we want to trigger the destructor
        /// only once: when all references to PublisherPrivate are out of scope.
        /// The destructor of PublisherPrivate unadvertise the topic.
        private: std::shared_ptr<PublisherPrivate> dataPtr;
      };

      /// \brief Constructor.
      /// \param[in] _options Node options.
      public: explicit Node(const NodeOptions &_options = NodeOptions());

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new topic. If a topic is currently advertised,
      /// you cannot advertise it a second time (regardless of its type).
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _options Advertise options.
      /// \return A PublisherId, which can be used in Node::Publish calls.
      /// The PublisherId also acts as boolean, where true occurs if the topic
      /// was succesfully advertised.
      /// \sa AdvertiseOptions.
      public: template<typename MessageT>
      Node::Publisher Advertise(
          const std::string &_topic,
          const AdvertiseMessageOptions &_options = AdvertiseMessageOptions())
      {
        return this->Advertise(_topic, MessageT().GetTypeName(), _options);
      }

      /// \brief Advertise a new topic. If a topic is currently advertised,
      /// you cannot advertise it a second time (regardless of its type).
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _msgTypeName Name of the message type that will be
      /// published on the topic. The message type name can be retrieved
      /// from a protobuf message using the GetTypeName() function.
      /// \param[in] _options Advertise options.
      /// \return A PublisherId, which can be used in Node::Publish calls.
      /// The PublisherId also acts as boolean, where true occurs if the topic
      /// was succesfully advertised.
      /// \sa AdvertiseOptions.
      public: Node::Publisher Advertise(
          const std::string &_topic,
          const std::string &_msgTypeName,
          const AdvertiseMessageOptions &_options = AdvertiseMessageOptions());

      /// \brief Get the list of topics advertised by this node.
      /// \return A vector containing all the topics advertised by this node.
      public: std::vector<std::string> AdvertisedTopics() const;

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback does not include any message information.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function with the following
      /// parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
          const std::string &_topic,
          void(*_cb)(const MessageT &_msg),
          const SubscribeOptions &_opts = SubscribeOptions())
      {
        std::function<void(const MessageT &, const MessageInfo &)> f =
          [_cb](const MessageT & _internalMsg,
                const MessageInfo &/*_internalInfo*/)
        {
          (*_cb)(_internalMsg);
        };

        return this->Subscribe<MessageT>(_topic, f, _opts);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback does not include any message information.
      /// In this version the callback is a lamda function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Lambda function with the following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
          const std::string &_topic,
          std::function<void(const MessageT &_msg)> &_cb,
          const SubscribeOptions &_opts = SubscribeOptions())
      {
        std::function<void(const MessageT &, const MessageInfo &)> f =
          [_cb](const MessageT & _internalMsg,
                const MessageInfo &/*_internalInfo*/)
        {
          _cb(_internalMsg);
        };

        return this->Subscribe<MessageT>(_topic, f, _opts);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback does not include any message information.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function with the following
      /// parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename ClassT, typename MessageT>
      bool Subscribe(
          const std::string &_topic,
          void(ClassT::*_cb)(const MessageT &_msg),
          ClassT *_obj,
          const SubscribeOptions &_opts = SubscribeOptions())
      {
        std::function<void(const MessageT &, const MessageInfo &)> f =
          [_cb, _obj](const MessageT & _internalMsg,
                      const MessageInfo &/*_internalInfo*/)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1);
          cb(_internalMsg);
        };

        return this->Subscribe<MessageT>(_topic, f, _opts);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback includes message information.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function with the following
      /// parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      ///   \param[in] _info Message information (e.g.: topic name).
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
          const std::string &_topic,
          void(*_cb)(const MessageT &_msg, const MessageInfo &_info),
          const SubscribeOptions &_opts = SubscribeOptions())
      {
        std::function<void(const MessageT &, const MessageInfo &)> f =
          [_cb](const MessageT & _internalMsg,
                const MessageInfo &_internalInfo)
        {
          (*_cb)(_internalMsg, _internalInfo);
        };

        return this->Subscribe<MessageT>(_topic, f, _opts);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback includes message information.
      /// In this version the callback is a lamda function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Lambda function with the following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      ///   \param[in] _info Message information (e.g.: topic name).
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
          const std::string &_topic,
          std::function<void(const MessageT &_msg,
                             const MessageInfo &_info)> &_cb,
          const SubscribeOptions &_opts = SubscribeOptions())
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        // Create a new subscription handler.
        std::shared_ptr<SubscriptionHandler<MessageT>> subscrHandlerPtr(
            new SubscriptionHandler<MessageT>(this->NodeUuid(), _opts));

        // Insert the callback into the handler.
        subscrHandlerPtr->SetCallback(_cb);

        std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

        // Store the subscription handler. Each subscription handler is
        // associated with a topic. When the receiving thread gets new data,
        // it will recover the subscription handler associated to the topic and
        // will invoke the callback.
        this->Shared()->localSubscribers.normal.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), subscrHandlerPtr);

        return this->SubscribeHelper(fullyQualifiedTopic);
      }

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback includes message information.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function with the following
      /// parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      ///   \param[in] _info Message information (e.g.: topic name).
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename ClassT, typename MessageT>
      bool Subscribe(
          const std::string &_topic,
          void(ClassT::*_cb)(const MessageT &_msg, const MessageInfo &_info),
          ClassT *_obj,
          const SubscribeOptions &_opts = SubscribeOptions())
      {
        std::function<void(const MessageT &, const MessageInfo &)> f =
          [_cb, _obj](const MessageT & _internalMsg,
                      const MessageInfo &_internalInfo)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1,
            std::placeholders::_2);
          cb(_internalMsg, _internalInfo);
        };

        return this->Subscribe<MessageT>(_topic, f, _opts);
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

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_cb)(const Request &_req, const Reply &_rep)} for
      /// advertising a service.
      /// \param[in] _topic Topic name associated with the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep ProtobufMessage containing the response.
      ///   \param[out] _result Service call result
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or false
      /// otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean.
      public: template<typename RequestT, typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
          const std::string &_topic,
          void(*_cb)(const RequestT &_req, ReplyT &_rep, bool &_result),
          const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT &, ReplyT&)> newCb =
          [=](const RequestT &_internalReq, ReplyT &_internalRep) -> bool
        {
          bool internalResult = false;
          (*_cb)(_internalReq, _internalRep, internalResult);
          return internalResult;
        };

        return this->Advertise(_topic, newCb, _options);
      }

      /// \brief Advertise a new service.
      /// In this version the callback is a plain function pointer.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(*_cb)(const RequestT &_req, ReplyT &_rep),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        // Dev Note: This overload of Advertise(~) is necessary so that the
        // compiler can correctly infer the template arguments. We cannot rely
        // on the compiler to implicitly cast the function pointer to a
        // std::function object, because the compiler cannot infer the template
        // parameters T1 and T2 from the signature of the function pointer that
        // gets passed to Advertise(~).

        // We create a std::function object so that we can explicitly call the
        // baseline overload of Advertise(~).
        std::function<bool(const RequestT&, ReplyT&)> f =
          [_cb](const RequestT &_internalReq, ReplyT &_internalRep)
        {
          return (*_cb)(_internalReq, _internalRep);
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_cb)(T &_rep)} for advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        void(*_cb)(ReplyT &_rep, bool &_result),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const msgs::Empty &, ReplyT &)> f =
          [_cb](const msgs::Empty &/*_internalReq*/, ReplyT &_internalRep)
        {
          bool internalResult = false;
          (*_cb)(_internalRep, internalResult);
          return internalResult;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service without input parameter.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(*_cb)(ReplyT &_rep),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const msgs::Empty &, ReplyT &)> f =
          [_cb](const msgs::Empty &/*_internalReq*/, ReplyT &_internalRep)
        {
          return (*_cb)(_internalRep);
        };
        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service without any output parameter.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT>
      bool Advertise(
        const std::string &_topic,
        void(*_cb)(const RequestT &_req),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT &, ignition::msgs::Empty &)> f =
          [_cb](const RequestT &_internalReq,
                ignition::msgs::Empty &/*_internalRep*/)
        {
          (*_cb)(_internalReq);
          return true;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_cb)(const T1 &_req, T2 &_rep)} for advertising a
      /// service.
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
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename RequestT, typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        std::function<void(const RequestT &_req,
                           ReplyT &_rep, bool &_result)> &_cb,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT&, ReplyT&)> f =
            [_cb](const RequestT &_req, ReplyT &_rep)
        {
          bool internalResult = false;
          (_cb)(_req, _rep, internalResult);
          return internalResult;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        std::function<bool(const RequestT &_req, ReplyT &_rep)> &_cb,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        // Create a new service reply handler.
        std::shared_ptr<RepHandler<RequestT, ReplyT>> repHandlerPtr(
          new RepHandler<RequestT, ReplyT>());

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
          this->Shared()->pUuid, this->NodeUuid(),
          RequestT().GetTypeName(), ReplyT().GetTypeName(), _options);

        if (!this->Shared()->AdvertisePublisher(publisher))
        {
          std::cerr << "Node::Advertise(): Error advertising service ["
                    << _topic
                    << "]. Did you forget to start the discovery service?"
                    << std::endl;
          return false;
        }

        return true;
      }

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_cb)(T2 &_rep)} for advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        std::function<void(ReplyT &_rep, bool &_result)> &_cb,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const msgs::Empty &, ReplyT &)> f =
          [_cb](const msgs::Empty &/*_internalReq*/, ReplyT &_internalRep)
        {
          bool internalResult = false;
          (_cb)(_internalRep, internalResult);
          return internalResult;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service without input parameter.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        std::function<bool(ReplyT &_rep)> &_cb,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const msgs::Empty &, ReplyT &)> f =
          [_cb](const msgs::Empty &/*_internalReq*/, ReplyT &_internalRep)
        {
          return (_cb)(_internalRep);
        };
        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service without any output parameter.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT>
      bool Advertise(
        const std::string &_topic,
        std::function<void(const RequestT &_req)> &_cb,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT &, ignition::msgs::Empty &)> f =
          [_cb](const RequestT &_internalReq,
                ignition::msgs::Empty &/*_internalRep*/)
        {
          (_cb)(_internalReq);
          return true;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (C::*_cb)(const T1 &_req, T2 &_rep)} for advertising a
      /// service.
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
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename ClassT, typename RequestT, typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        void(ClassT::*_cb)(const RequestT &_req, ReplyT &_rep, bool &_result),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT &, ReplyT &)> f =
          [_cb, _obj](const RequestT &_internalReq,
                      ReplyT &_internalRep)
        {
          bool internalResult;
          (_obj->*_cb)(_internalReq, _internalRep, internalResult);
          return internalResult;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ClassT, typename RequestT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(ClassT::*_cb)(const RequestT &_req, ReplyT &_rep),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT &, ReplyT &)> f =
          [_cb, _obj](const RequestT &_internalReq,
                      ReplyT &_internalRep)
        {
          return (_obj->*_cb)(_internalReq, _internalRep);
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (C::*_cb)(T &_rep)} for advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename ClassT, typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        void(ClassT::*_cb)(ReplyT &_rep, bool &_result),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const msgs::Empty &, ReplyT &)> f =
          [_cb, _obj](const msgs::Empty &/*_internalReq*/, ReplyT &_internalRep)
        {
          bool internalResult;
          (_obj->*_cb)(_internalRep, internalResult);
          return internalResult;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service without input parameter.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _rep Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ClassT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(ClassT::*_cb)(ReplyT &_rep),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const msgs::Empty &, ReplyT &)> f =
          [_cb, _obj](const msgs::Empty &/*_internalReq*/, ReplyT &_internalRep)
        {
          return (_obj->*_cb)(_internalRep);
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Advertise a new service without any output parameter.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _cb Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _req Protobuf message containing the request.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions
      public: template<typename ClassT, typename RequestT>
      bool Advertise(
        const std::string &_topic,
        void(ClassT::*_cb)(const RequestT &_req),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions())
      {
        std::function<bool(const RequestT &, ignition::msgs::Empty &)> f =
          [_cb, _obj](const RequestT &_internalReq,
             ignition::msgs::Empty &/*_internalRep*/)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1);
          cb(_internalReq);
          return true;
        };

        return this->Advertise(_topic, f, _options);
      }

      /// \brief Get the list of services advertised by this node.
      /// \return A vector containing all services advertised by this node.
      public: std::vector<std::string> AdvertisedServices() const;

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives. The callback has the following parameters:
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_req,
        void(*_cb)(const ReplyT &_rep, const bool _result))
      {
        std::function<void(const ReplyT &, const bool)> f =
          [_cb](const ReplyT &_internalRep, const bool _internalResult)
        {
          (*_cb)(_internalRep, _internalResult);
        };

        return this->Request<RequestT, ReplyT>(_topic, _req, f);
      }

      /// \brief Request a new service without input parameter using a
      /// non-blocking call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives. The callback has the following parameters:
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename ReplyT>
      bool Request(
        const std::string &_topic,
        void(*_cb)(const ReplyT &_rep, const bool _result))
      {
        msgs::Empty req;
        return this->Request(_topic, req, _cb);
      }

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _cb Lambda function executed when the response arrives.
      /// The callback has the following parameters:
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_req,
        std::function<void(const ReplyT &_rep, const bool _result)> &_cb)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        bool localResponserFound;
        IRepHandlerPtr repHandler;
        {
          std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);
          localResponserFound = this->Shared()->repliers.FirstHandler(
                fullyQualifiedTopic,
                RequestT().GetTypeName(),
                ReplyT().GetTypeName(),
                repHandler);
        }

        // If the responser is within my process.
        if (localResponserFound)
        {
          // There is a responser in my process, let's use it.
          ReplyT rep;
          bool result = repHandler->RunLocalCallback(_req, rep);

          _cb(rep, result);
          return true;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<RequestT, ReplyT>> reqHandlerPtr(
          new ReqHandler<RequestT, ReplyT>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(&_req);

        // Insert the callback into the handler.
        reqHandlerPtr->SetCallback(_cb);

        {
          std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

          // Store the request handler.
          this->Shared()->requests.AddHandler(
            fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

          // If the responser's address is known, make the request.
          SrvAddresses_M addresses;
          if (this->Shared()->TopicPublishers(fullyQualifiedTopic, addresses))
          {
            this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic,
              RequestT().GetTypeName(), ReplyT().GetTypeName());
          }
          else
          {
            // Discover the service responser.
            if (!this->Shared()->DiscoverService(fullyQualifiedTopic))
            {
              std::cerr << "Node::Request(): Error discovering service ["
                        << _topic
                        << "]. Did you forget to start the discovery service?"
                        << std::endl;
              return false;
            }
          }
        }

        return true;
      }

      /// \brief Request a new service without input parameter using a
      /// non-blocking call.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _cb Lambda function executed when the response arrives.
      /// The callback has the following parameters:
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename ReplyT>
      bool Request(
        const std::string &_topic,
        std::function<void(const ReplyT &_rep, const bool _result)> &_cb)
      {
        msgs::Empty req;
        return this->Request(_topic, req, _cb);
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
      public: template<typename ClassT, typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_req,
        void(ClassT::*_cb)(const ReplyT &_rep, const bool _result),
        ClassT *_obj)
      {
        std::function<void(const ReplyT &, const bool)> f =
          [_cb, _obj](const ReplyT &_internalRep, const bool _internalResult)
        {
          auto cb = std::bind(_cb, _obj, std::placeholders::_1,
            std::placeholders::_2);
          cb(_internalRep, _internalResult);
        };

        return this->Request<RequestT, ReplyT>(_topic, _req, f);
      }

      /// \brief Request a new service without input parameter using a
      /// non-blocking call.
      /// In this version the callback is a member function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _cb Pointer to the callback function executed when the
      /// response arrives. The callback has the following parameters:
      ///   \param[in] _rep Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when the service call was succesfully requested.
      public: template<typename ClassT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        void(ClassT::*_cb)(const ReplyT &_rep, const bool _result),
        ClassT *_obj)
      {
        msgs::Empty req;
        return this->Request(_topic, req, _cb, _obj);
      }

      /// \brief Request a new service using a blocking call.
      /// \param[in] _topic Service name requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \param[in] _timeout The request will timeout after '_timeout' ms.
      /// \param[out] _rep Protobuf message containing the response.
      /// \param[out] _result Result of the service call.
      /// \return true when the request was executed or false if the timeout
      /// expired.
      public: template<typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_req,
        const unsigned int &_timeout,
        ReplyT &_rep,
        bool &_result)
      {
        std::string fullyQualifiedTopic;
        if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
          this->Options().NameSpace(), _topic, fullyQualifiedTopic))
        {
          std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
          return false;
        }

        // Create a new request handler.
        std::shared_ptr<ReqHandler<RequestT, ReplyT>> reqHandlerPtr(
          new ReqHandler<RequestT, ReplyT>(this->NodeUuid()));

        // Insert the request's parameters.
        reqHandlerPtr->SetMessage(&_req);
        reqHandlerPtr->SetResponse(&_rep);

        std::unique_lock<std::recursive_mutex> lk(this->Shared()->mutex);

        // If the responser is within my process.
        IRepHandlerPtr repHandler;
        if (this->Shared()->repliers.FirstHandler(fullyQualifiedTopic,
          _req.GetTypeName(), _rep.GetTypeName(), repHandler))
        {
          // There is a responser in my process, let's use it.
          _result = repHandler->RunLocalCallback(_req, _rep);
          return true;
        }

        // Store the request handler.
        this->Shared()->requests.AddHandler(
          fullyQualifiedTopic, this->NodeUuid(), reqHandlerPtr);

        // If the responser's address is known, make the request.
        SrvAddresses_M addresses;
        if (this->Shared()->TopicPublishers(fullyQualifiedTopic, addresses))
        {
          this->Shared()->SendPendingRemoteReqs(fullyQualifiedTopic,
            _req.GetTypeName(), _rep.GetTypeName());
        }
        else
        {
          // Discover the service responser.
          if (!this->Shared()->DiscoverService(fullyQualifiedTopic))
          {
            std::cerr << "Node::Request(): Error discovering service ["
                      << _topic
                      << "]. Did you forget to start the discovery service?"
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

      /// \brief Request a new service without input parameter using a blocking
      /// call.
      /// \param[in] _topic Service name requested.
      /// \param[in] _timeout The request will timeout after '_timeout' ms.
      /// \param[out] _rep Protobuf message containing the response.
      /// \param[out] _result Result of the service call.
      /// \return true when the request was executed or false if the timeout
      /// expired.
      public: template<typename ReplyT>
      bool Request(
        const std::string &_topic,
        const unsigned int &_timeout,
        ReplyT &_rep,
        bool &_result)
      {
        msgs::Empty req;
        return this->Request(_topic, req, _timeout, _rep, _result);
      }

      /// \brief Request a new service without waiting for response.
      /// \param[in] _topic Topic requested.
      /// \param[in] _req Protobuf message containing the request's parameters.
      /// \return true when the service call was succesfully requested.
      public: template<typename RequestT>
      bool Request(
          const std::string &_topic,
          const RequestT &_req)
      {
        // This callback is here for reusing the regular Request() call with
        // input and output parameters.
        std::function<void(const ignition::msgs::Empty &, const bool)> f =
          [](const ignition::msgs::Empty &, const bool)
        {
        };

        return this->Request<RequestT, ignition::msgs::Empty>(_topic, _req, f);
      }

      /// \brief Unadvertise a service.
      /// \param[in] _topic Service name to be unadvertised.
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
      /// \param[out] _services List of advertised services.
      public: void ServiceList(std::vector<std::string> &_services) const;

      /// \brief Get the information about a service.
      /// \param[in] _service Name of the service.
      /// \param[out] _publishers List of publishers on the service.
      /// \return False if unable to get service info.
      public: bool ServiceInfo(const std::string &_service,
                              std::vector<ServicePublisher> &_publishers) const;

      /// \brief Subscribe to a topic registering a callback. The callback must
      /// accept a std::string to represent the message data, and a MessageInfo
      /// which provides metadata about the message.
      /// \param[in] _topic Name of the topic to subscribe to
      /// \param[in] _callback A function pointer or std::function object that
      /// has a void return value and accepts two arguments:
      /// (const std::string &_msgData, const MessageInfo &_info).
      /// \param[in] _msgType The type of message to subscribe to. Using
      /// kGenericMessageType (the default) will allow this subscriber to listen
      /// to all message types. The callback function can identify the type for
      /// each message by inspecting its const MessageInfo& input argument.
      /// \param[in] _opts Options for subscribing.
      /// \return True if subscribing was successful.
      public: bool SubscribeRaw(
        const std::string &_topic,
        const RawCallback &_callback,
        const std::string &_msgType = kGenericMessageType,
        const SubscribeOptions &_opts = SubscribeOptions());

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

      /// \brief Get the set of topics subscribed by this node.
      /// \return The set of subscribed topics.
      private: std::unordered_set<std::string> &TopicsSubscribed() const;

      /// \brief Get the set of services advertised by this node.
      /// \return The set of advertised services.
      private: std::unordered_set<std::string> &SrvsAdvertised() const;

      /// \brief Get the reference to the current node options.
      /// \return Reference to the current node options.
      private: NodeOptions &Options() const;

      /// \brief Helper function for Subscribe.
      /// \param[in] _fullyQualifiedTopic Fully qualified topic name
      /// \return True on success.
      private: bool SubscribeHelper(const std::string &_fullyQualifiedTopic);

      /// \internal
      /// \brief Smart pointer to private data.
      private: std::unique_ptr<transport::NodePrivate> dataPtr;
    };
  }
}
#endif
