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

#ifndef _IGN_TRANSPORT_NODESHARED_HH_INCLUDED__
#define _IGN_TRANSPORT_NODESHARED_HH_INCLUDED__

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#include <zmq.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ignition/transport/Discovery.hh"
#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

namespace ignition
{
  namespace transport
  {
    /// \class NodeShared NodeShared.hh ignition/transport/NodeShared.hh
    /// \brief Private data for the Node class. This class should not be
    /// directly used. You should use the Node class.
    class IGNITION_VISIBLE NodeShared
    {
      /// \brief NodeShared is a singleton. This method gets the
      /// NodeShared instance shared between all the nodes.
      /// \return Pointer to the current NodeShared instance.
      public: static NodeShared *Instance();

      /// \brief Receive data and control messages.
      public: void RunReceptionTask();

      /// \brief Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _data Data to publish.
      /// \param[in] _msgType Message type in string format.
      /// \return true when success or false otherwise.
      public: bool Publish(const std::string &_topic,
                           const std::string &_data,
                           const std::string &_msgType);

      /// \brief Method in charge of receiving the topic updates.
      public: void RecvMsgUpdate();

      /// \brief Method in charge of receiving the control updates (when a new
      /// remote subscriber notifies its presence for example).
      public: void RecvControlUpdate();

      /// \brief Method in charge of receiving the service call requests.
      public: void RecvSrvRequest();

      /// \brief Method in charge of receiving the service call responses.
      public: void RecvSrvResponse();

      /// \brief Try to send all the requests for a given service call and a
      /// pair of request/response types.
      /// \param[in] _topic Topic name.
      /// \param[in] _reqType Type of the request in string format.
      /// \param[in] _repType Type of the response in string format.
      public: void SendPendingRemoteReqs(const std::string &_topic,
                                         const std::string &_reqType,
                                         const std::string &_repType);

      /// \brief Callback executed when the discovery detects new topics.
      /// \param[in] _pub Information of the publisher in charge of the topic.
      public: void OnNewConnection(const MessagePublisher &_pub);

      /// \brief Callback executed when the discovery detects disconnections.
      /// \param[in] _pub Information of the publisher in charge of the topic.
      public: void OnNewDisconnection(const MessagePublisher &_pub);

      /// \brief Callback executed when the discovery detects a new service call
      /// \param[in] _pub Information of the publisher in charge of the service.
      public: void OnNewSrvConnection(const ServicePublisher &_pub);

      /// \brief Callback executed when a service call is no longer available.
      /// \param[in] _pub Information of the publisher in charge of the service.
      public: void OnNewSrvDisconnection(const ServicePublisher &_pub);

      /// \brief Constructor.
      protected: NodeShared();

      /// \brief Destructor.
      protected: virtual ~NodeShared();

      /// \brief Timeout used for receiving messages (ms.).
      public: static const int Timeout = 250;

      /// \brief Print activity to stdout.
      public: int verbose;

      /// \brief My pub/sub address.
      public: std::string myAddress;

      /// \brief My pub/sub control address.
      public: std::string myControlAddress;

      /// \brief My requester service call address.
      public: std::string myRequesterAddress;

      /// \brief My replier service call address.
      public: std::string myReplierAddress;

      /// \brief IP address of this host.
      public: std::string hostAddr;

      /// \brief Discovery service.
      public: std::unique_ptr<Discovery> discovery;

      /// \brief 0MQ context.
      public: zmq::context_t *context;

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisher;

      /// \brief ZMQ socket to receive topic updates.
      public: std::unique_ptr<zmq::socket_t> subscriber;

      /// \brief ZMQ socket to receive control updates (new connections, ...).
      public: std::unique_ptr<zmq::socket_t> control;

      /// \brief ZMQ socket for sending service call requests.
      public: std::unique_ptr<zmq::socket_t> requester;

      /// \brief ZMQ socket for receiving service call responses.
      public: std::unique_ptr<zmq::socket_t> responseReceiver;

      /// \brief Response receiver socket identity.
      public: Uuid responseReceiverId;

      /// \brief Replier socket identity.
      public: Uuid replierId;

      /// \brief ZMQ socket to receive service call requests.
      public: std::unique_ptr<zmq::socket_t> replier;

      /// \brief Process UUID.
      public: std::string pUuid;

      /// \brief Timeout used for receiving requests.
      public: int timeout;

      /// \brief thread in charge of receiving and handling incoming messages.
      public: std::thread threadReception;

      /// \brief Mutex to guarantee exclusive access between all threads.
      public: std::recursive_mutex mutex;

      /// \brief When true, the reception thread will finish.
      public: bool exit;

#ifdef _WIN32
      /// \brief True when the reception thread is finishing.
      public: bool threadReceptionExiting;
#endif

      /// \brief Mutex to guarantee exclusive access to the 'exit' variable.
      private: std::mutex exitMutex;

      /// \brief Remote connections for pub/sub messages.
      private: TopicStorage<MessagePublisher> connections;

      /// \brief List of connected zmq end points for request/response.
      private: std::vector<std::string> srvConnections;

      /// \brief Remote subscribers.
      public: TopicStorage<MessagePublisher> remoteSubscribers;

      /// \brief Subscriptions.
      public: HandlerStorage<ISubscriptionHandler> localSubscriptions;

      /// \brief Service call repliers.
      public: HandlerStorage<IRepHandler> repliers;

      /// \brief Pending service call requests.
      public: HandlerStorage<IReqHandler> requests;
    };
  }
}
#endif
