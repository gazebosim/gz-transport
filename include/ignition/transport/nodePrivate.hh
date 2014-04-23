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

#ifndef _NODE_PRIVATE_HH_
#define _NODE_PRIVATE_HH_

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
    /// \brief public data for the Node class.
    class NodePrivate : public Singleton<NodePrivate>
    {
      /// \brief Constructor.
      /// \param[in] _verbose true for enabling verbose mode.
      public: NodePrivate (bool _verbose);

      /// \brief Destructor.
      public: virtual ~NodePrivate();

      /// \brief Run one iteration of the transport.
      public: void SpinOnce();

      /// \brief Receive messages forever.
      public: void Spin();

      /// \brief Deallocate resources.
      public: void Fini();

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdates();

      /// \brief Method in charge of receiving the topic updates.
      public: void RecvTopicUpdates();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      public: int DispatchDiscoveryMsg(char *_msg);

      /// \brief Send an ADVERTISE message to the discovery socket.
      /// \param[in] _type ADV or ADV_SVC.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _address Address to be advertised with the topic.
      /// \return 0 when success.
      public: int SendAdvertiseMsg(uint8_t _type, const std::string &_topic,
                                    const std::string &_address);

      /// \brief Send a SUBSCRIBE message to the discovery socket.
      /// \param[in] _type SUB or SUB_SVC.
      /// \param[in] _topic Topic name.
      /// \return 0 when success.
      public: int SendSubscribeMsg(uint8_t _type, const std::string &_topic);

      /// \brief Print activity to stdout.
      public: int verbose;

      /// \brief Topic information.
      public: TopicsInfo topics;

      /// \brief My pub/sub address.
      public: std::vector<std::string> myAddresses;

      /// \brief IP address of this host.
      public: std::string hostAddr;

      /// \brief Broadcast IP address.
      public: std::string bcastAddr;

      /// \brief UDP broadcast port used for the transport.
      public: int bcastPort;

      /// \brief UDP socket used for receiving discovery messages.
      public: UDPSocket *bcastSockIn;

      /// \brief UDP socket used for sending discovery messages.
      public: UDPSocket *bcastSockOut;

      /// \brief 0MQ context.
      public: zmq::context_t *context;

      /// \brief ZMQ socket to send topic updates.
      public: zmq::socket_t *publisher;

      /// \brief ZMQ socket to receive topic updates.
      public: zmq::socket_t *subscriber;

      /// \brief ZMQ tcp local endpoint.
      public: std::string tcpEndpoint;

      /// \brief Local GUID.
      public: uuid_t guid;

      /// \brief String conversion of the GUID.
      public: std::string guidStr;

      /// \brief Timeout used for receiving requests.
      public: int timeout;

      /// \brief thread in charge of receiving and handling incoming messages.
      public: std::thread *threadInbound;

      /// \brief Mutex to guarantee exclusive access between the inbound and
      /// outbound thread.
      public: std::mutex mutex;
    };
  }
}
#endif
