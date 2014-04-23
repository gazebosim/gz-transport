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

#ifndef __NODE_HH_INCLUDED__
#define __NODE_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <uuid/uuid.h>
#include <mutex>
#include <string>
#include <thread>
#include <zmq.hpp>
#include "ignition/transport/packet.hh"
#include "ignition/transport/singleton.hh"
#include "ignition/transport/socket.hh"
#include "ignition/transport/topicsInfo.hh"

namespace ignition
{
  namespace transport
  {
    /// \brief Longest string to receive.
    const int MaxRcvStr = 65536;

    /// \brief ZMQ endpoint used for inproc communication.
    const std::string InprocAddr = "inproc://local";

    class Node : public Singleton<Node>
    {
      /// \brief Constructor.
      /// \param[in] _master End point with the master's endpoint.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Node (const std::string &_master, bool _verbose);

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new service.
      /// \param[in] _topic Topic to be advertised.
      /// \return 0 when success.
      public: int Advertise(const std::string &_topic);

      /// \brief Unadvertise a new service.
      /// \param[in] _topic Topic to be unadvertised.
      /// \return 0 when success.
      public: int UnAdvertise(const std::string &_topic);

      /// \brief Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _data Data to publish.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic, const std::string &_data);

      /// \brief Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _message protobuf message.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic,
                          const google::protobuf::Message &_message);

      /// \brief Subscribe to a topic registering a callback.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function.
      /// \return 0 when success.
      public: int Subscribe(const std::string &_topic,
                          void(*_cb)(const std::string &, const std::string &));

      /// \brief Subscribe to a topic registering a callback.
      /// \param[in] _topic Topic to be unsubscribed.
      /// \return 0 when success.
      public: int UnSubscribe(const std::string &_topic);

      /// \brief Advertise a new service call registering a callback.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _cb Pointer to the callback function.
      /// \return 0 when success.
      public: int SrvAdvertise(const std::string &_topic,
        int(*_cb)(const std::string &, const std::string &, std::string &));

      /// \brief Unadvertise a service call registering a callback.
      /// \param[in] _topic Topic to be unadvertised.
      /// \return 0 when success.
      public: int SrvUnAdvertise(const std::string &_topic);

      /// \brief Request a new service to another component using a blocking call.
      /// \param[in] _topic Topic requested.
      /// \param[in] _data Data of the request.
      /// \param[out] _response Response of the request.
      /// \return 0 when success.
      public: int SrvRequest(const std::string &_topic,
                             const std::string &_data,
                             std::string &_response);

      /// \brief Request a new service call using a non-blocking call.
      /// \param[in] _topic Topic requested.
      /// \param[in] _data Data of the request.
      /// \param[in] _cb Pointer to the callback function.
      /// \return 0 when success.
      public: int SrvRequestAsync(const std::string &_topic,
                                  const std::string &_data,
        void(*_cb)(const std::string &_topic, int rc, const std::string &_rep));

      /// \brief Run one iteration of the transport.
      private: void SpinOnce();

      /// \brief Receive messages forever.
      private: void Spin();

      /// \brief Deallocate resources.
      private: void Fini();

      /// \brief Method in charge of receiving the discovery updates.
      private: void RecvDiscoveryUpdates();

      /// \brief Method in charge of receiving the topic updates.
      private: void RecvTopicUpdates();

      /// \brief Method in charge of receiving the service call requests.
      private: void RecvSrvRequest();

      /// \brief Method in charge of receiving the async service call replies.
      private: void RecvSrvReply();

      /// \brief Send all the pendings asynchronous service calls (if possible)
      private: void SendPendingAsyncSrvCalls();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      private: int DispatchDiscoveryMsg(char *_msg);

      /// \brief Send an ADVERTISE message to the discovery socket.
      /// \param[in] _type ADV or ADV_SVC.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _address Address to be advertised with the topic.
      /// \return 0 when success.
      private: int SendAdvertiseMsg(uint8_t _type, const std::string &_topic,
                                    const std::string &_address);

      /// \brief Send a SUBSCRIBE message to the discovery socket.
      /// \param[in] _type SUB or SUB_SVC.
      /// \param[in] _topic Topic name.
      /// \return 0 when success.
      private: int SendSubscribeMsg(uint8_t _type, const std::string &_topic);

      /// \brief Master address.
      private: std::string master;

      /// \brief Print activity to stdout.
      private: int verbose;

      /// \brief Topic information.
      private: TopicsInfo topics;

      /// \brief Topic information for service calls.
      private: TopicsInfo topicsSrvs;

      /// \brief My pub/sub address.
      private: std::vector<std::string> myAddresses;

      /// \brief My req/rep address
      private: std::vector<std::string> mySrvAddresses;

      /// \brief IP address of this host.
      private: std::string hostAddr;

      /// \brief Broadcast IP address.
      private: std::string bcastAddr;

      /// \brief UDP broadcast port used for the transport.
      private: int bcastPort;

      /// \brief UDP socket used for the discovery protocol.
      private: UDPSocket *bcastSockIn;

      private: UDPSocket *bcastSockOut;

      /// \brief 0MQ context.
      private: zmq::context_t *context;

      /// \brief ZMQ socket to send topic updates.
      private: zmq::socket_t *publisher;

      /// \brief ZMQ socket to receive topic updates.
      private: zmq::socket_t *subscriber;

      /// \brief ZMQ socket to send service call requests.
      private: zmq::socket_t *srvRequester;

      /// \brief ZMQ socket to receive service call requests.
      private: zmq::socket_t *srvReplier;

      /// \brief ZMQ tcp local endpoint.
      private: std::string tcpEndpoint;

      /// \brief ZMQ endpoint used by a service call requester.
      private: std::string srvRequesterEP;

      /// \brief ZMQ endpoing used to answer the service calls.
      private: std::string srvReplierEP;

      /// \brief Timeout used for the blocking service requests.
      private: int timeout;

      /// \brief Local GUID.
      private: uuid_t guid;

      /// \brief String conversion of the GUID.
      private: std::string guidStr;

      private: std::thread *inThread;

      private: std::mutex mutex;
    };
  }
}
#endif
