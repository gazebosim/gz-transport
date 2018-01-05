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
#include "ignition/transport/Helpers.hh"
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
    IGNITION_TRANSPORT_VISIBLE void waitForShutdown();

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
      ///      pub.Publish(msg);
      ///    }
      public: class Publisher
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

        /// \brief Publish a message.
        /// \param[in] _msg A google::protobuf message.
        /// \return true when success.
        public: bool Publish(const ProtoMsg &_msg);

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
        /// only once when all references to PublisherPrivate are out of scope.
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
        const AdvertiseMessageOptions &_options = AdvertiseMessageOptions());

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
      /// \param[in] _callback Pointer to the callback function with the
      /// following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
        const std::string &_topic,
        void(*_callback)(const MessageT &_msg),
        const SubscribeOptions &_opts = SubscribeOptions());

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback does not include any message information.
      /// In this version the callback is a lamda function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _callback Lambda function with the following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
        const std::string &_topic,
        std::function<void(const MessageT &_msg)> &_callback,
        const SubscribeOptions &_opts = SubscribeOptions());

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback does not include any message information.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _callback Pointer to the callback function with the
      /// following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename ClassT, typename MessageT>
      bool Subscribe(
        const std::string &_topic,
        void(ClassT::*_callback)(const MessageT &_msg),
        ClassT *_obj,
        const SubscribeOptions &_opts = SubscribeOptions());

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback includes message information.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _callback Pointer to the callback function with the
      /// following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      ///   \param[in] _info Message information (e.g.: topic name).
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
        const std::string &_topic,
        void(*_callback)(const MessageT &_msg, const MessageInfo &_info),
        const SubscribeOptions &_opts = SubscribeOptions());

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback includes message information.
      /// In this version the callback is a lamda function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _callback Lambda function with the following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      ///   \param[in] _info Message information (e.g.: topic name).
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename MessageT>
      bool Subscribe(
        const std::string &_topic,
        std::function<void(const MessageT &_msg,
                           const MessageInfo &_info)> &_callback,
        const SubscribeOptions &_opts = SubscribeOptions());

      /// \brief Subscribe to a topic registering a callback.
      /// Note that this callback includes message information.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _callback Pointer to the callback function with the
      /// following parameters:
      ///   \param[in] _msg Protobuf message containing a new topic update.
      ///   \param[in] _info Message information (e.g.: topic name).
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _opts Subscription options.
      /// \return true when successfully subscribed or false otherwise.
      public: template<typename ClassT, typename MessageT>
      bool Subscribe(
        const std::string &_topic,
        void(ClassT::*_callback)(const MessageT &_msg,
                                 const MessageInfo &_info),
        ClassT *_obj,
        const SubscribeOptions &_opts = SubscribeOptions());

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
      /// \code{bool (*_callback)(const Request &_request, const Reply &_reply)}
      /// for advertising a service.
      /// \param[in] _topic Topic name associated with the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      ///   \param[out] _reply ProtobufMessage containing the response.
      ///   \param[out] _result Service call result
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or false
      /// otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean.
      public: template<typename RequestT, typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        void(*_callback)(const RequestT &_request,
                         ReplyT &_reply, bool &_result),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service.
      /// In this version the callback is a plain function pointer.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(*_callback)(const RequestT &_request, ReplyT &_reply),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_callback)(T &_reply)} for advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        void(*_callback)(ReplyT &_reply, bool &_result),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service without input parameter.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(*_callback)(ReplyT &_reply),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service without any output parameter.
      /// In this version the callback is a free function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT>
      bool Advertise(
        const std::string &_topic,
        void(*_callback)(const RequestT &_request),
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_callback)(const T1 &_request, T2 &_reply)} for
      /// advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename RequestT, typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        std::function<void(const RequestT &_request,
                           ReplyT &_reply, bool &_result)> &_callback,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        std::function<bool(const RequestT &_request, ReplyT &_reply)> _callback,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (*_callback)(T2 &_reply)} for advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \param[out] _result Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      /// \deprecated See version where the callback function returns a boolean
      public: template<typename ReplyT>
      IGN_DEPRECATED(4.0) bool Advertise(
        const std::string &_topic,
        std::function<void(ReplyT &_reply, bool &_result)> &_callback,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service without input parameter.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        std::function<bool(ReplyT &_reply)> &_callback,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service without any output parameter.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename RequestT>
      bool Advertise(
        const std::string &_topic,
        std::function<void(const RequestT &_request)> &_callback,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (C::*_callback)(const T1 &_request, T2 &_reply)} for
      /// advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      ///   \param[out] _reply Protobuf message containing the response.
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
        void(ClassT::*_callback)(const RequestT &_request,
                           ReplyT &_reply, bool &_result),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ClassT, typename RequestT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(ClassT::*_callback)(const RequestT &_request, ReplyT &_reply),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Old method for advertising a service. This signature is
      /// considered deprecated. Please migrate to the callback signature
      /// \code{bool (C::*_callback)(T &_reply)} for advertising a service.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _reply Protobuf message containing the response.
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
        void(ClassT::*_callback)(ReplyT &_reply, bool &_result),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service without input parameter.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[out] _reply Protobuf message containing the response.
      ///   \return Service call result.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions.
      public: template<typename ClassT, typename ReplyT>
      bool Advertise(
        const std::string &_topic,
        bool(ClassT::*_callback)(ReplyT &_reply),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Advertise a new service without any output parameter.
      /// In this version the callback is a member function.
      /// \param[in] _topic Topic name associated to the service.
      /// \param[in] _callback Callback to handle the service request with the
      /// following parameters:
      ///   \param[in] _request Protobuf message containing the request.
      /// \param[in] _obj Instance containing the member function.
      /// \param[in] _options Advertise options.
      /// \return true when the topic has been successfully advertised or
      /// false otherwise.
      /// \sa AdvertiseOptions
      public: template<typename ClassT, typename RequestT>
      bool Advertise(
        const std::string &_topic,
        void(ClassT::*_callback)(const RequestT &_request),
        ClassT *_obj,
        const AdvertiseServiceOptions &_options = AdvertiseServiceOptions());

      /// \brief Get the list of services advertised by this node.
      /// \return A vector containing all services advertised by this node.
      public: std::vector<std::string> AdvertisedServices() const;

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _request Protobuf message containing the request's
      /// parameters.
      /// \param[in] _callback Pointer to the callback function executed when
      /// the response arrives. The callback has the following parameters:
      ///   \param[in] _reply Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_request,
        void(*_callback)(const ReplyT &_reply, const bool _result));

      /// \brief Request a new service without input parameter using a
      /// non-blocking call.
      /// In this version the callback is a free function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _callback Pointer to the callback function executed when
      /// the response arrives. The callback has the following parameters:
      ///   \param[in] _reply Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename ReplyT>
      bool Request(
        const std::string &_topic,
        void(*_callback)(const ReplyT &_reply, const bool _result));

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _request Protobuf message containing the request's
      /// parameters.
      /// \param[in] _callback Lambda function executed when the response
      /// arrives. The callback has the following parameters:
      ///   \param[in] _reply Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_request,
        std::function<void(const ReplyT &_reply,
                           const bool _result)> &_callback);

      /// \brief Request a new service without input parameter using a
      /// non-blocking call.
      /// In this version the callback is a lambda function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _callback Lambda function executed when the response
      /// arrives. The callback has the following parameters:
      ///   \param[in] _reply Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \return true when the service call was succesfully requested.
      public: template<typename ReplyT>
      bool Request(
        const std::string &_topic,
        std::function<void(const ReplyT &_reply,
                           const bool _result)> &_callback);

      /// \brief Request a new service using a non-blocking call.
      /// In this version the callback is a member function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _request Protobuf message containing the request's
      /// parameters.
      /// \param[in] _callback Pointer to the callback function executed when
      /// the response arrives. The callback has the following parameters:
      ///   \param[in] _reply Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when the service call was succesfully requested.
      public: template<typename ClassT, typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_request,
        void(ClassT::*_callback)(const ReplyT &_reply, const bool _result),
        ClassT *_obj);

      /// \brief Request a new service without input parameter using a
      /// non-blocking call.
      /// In this version the callback is a member function.
      /// \param[in] _topic Service name requested.
      /// \param[in] _callback Pointer to the callback function executed when
      /// the response arrives. The callback has the following parameters:
      ///   \param[in] _reply Protobuf message containing the response.
      ///   \param[in] _result Result of the service call. If false, there was
      ///   a problem executing your request.
      /// \param[in] _obj Instance containing the member function.
      /// \return true when the service call was succesfully requested.
      public: template<typename ClassT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        void(ClassT::*_callback)(const ReplyT &_reply, const bool _result),
        ClassT *_obj);

      /// \brief Request a new service using a blocking call.
      /// \param[in] _topic Service name requested.
      /// \param[in] _request Protobuf message containing the request's
      /// parameters.
      /// \param[in] _timeout The request will timeout after '_timeout' ms.
      /// \param[out] _reply Protobuf message containing the response.
      /// \param[out] _result Result of the service call.
      /// \return true when the request was executed or false if the timeout
      /// expired.
      public: template<typename RequestT, typename ReplyT>
      bool Request(
        const std::string &_topic,
        const RequestT &_request,
        const unsigned int &_timeout,
        ReplyT &_reply,
        bool &_result);

      /// \brief Request a new service without input parameter using a blocking
      /// call.
      /// \param[in] _topic Service name requested.
      /// \param[in] _timeout The request will timeout after '_timeout' ms.
      /// \param[out] _reply Protobuf message containing the response.
      /// \param[out] _result Result of the service call.
      /// \return true when the request was executed or false if the timeout
      /// expired.
      public: template<typename ReplyT>
      bool Request(
        const std::string &_topic,
        const unsigned int &_timeout,
        ReplyT &_reply,
        bool &_result);

      /// \brief Request a new service without waiting for response.
      /// \param[in] _topic Topic requested.
      /// \param[in] _request Protobuf message containing the request's
      /// parameters.
      /// \return true when the service call was succesfully requested.
      public: template<typename RequestT>
      bool Request(const std::string &_topic, const RequestT &_request);

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
      public: bool ServiceInfo(
        const std::string &_service,
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

#include "ignition/transport/detail/Node.hh"

#endif
