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

#ifndef _IGN_TRANSPORT_NODEPRIVATE_HH_
#define _IGN_TRANSPORT_NODEPRIVATE_HH_

#include <google/protobuf/message.h>
#include <czmq.h>
#include <uuid/uuid.h>
#include <zmq.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ignition/transport/socket.hh"
#include "ignition/transport/TopicsInfo.hh"

namespace ignition
{
  namespace transport
  {
    #define BEACON_VERSION 0x01

    typedef struct {
      byte protocol[3];
      byte version;
      byte uuid[ZUUID_LEN];
      uint16_t port;
    } beacon_t;

    /// \brief Longest string to receive.
    const int MaxRcvStr = 65536;

    /// \brief public data for the Node class.
    class NodePrivate
    {
      public: static NodePrivatePtr GetInstance(bool _verbose);

      /// \brief Constructor.
      /// \param[in] _verbose true for enabling verbose mode.
      public: NodePrivate(bool _verbose);

      /// \brief Destructor.
      public: virtual ~NodePrivate();

      /// \brief Run one iteration of the transport.
      private: void SpinOnce();

      /// \brief Receive messages forever.
      private: void Spin();

      /// \brief Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _data Data to publish.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic, const std::string &_data);

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdate();

      /// \brief Method in charge of receiving the topic updates.
      public: void RecvMsgUpdate();

      private: void RecvBeaconUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      private: int DispatchDiscoveryMsg(char *_msg);

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
      public: std::unique_ptr<UDPSocket> bcastSockIn;

      /// \brief UDP socket used for sending discovery messages.
      public: std::unique_ptr<UDPSocket> bcastSockOut;

      /// \brief 0MQ context.
      public: std::unique_ptr<zmq::context_t> context;

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisher;

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisherLocal;

      /// \brief ZMQ socket to receive topic updates.
      public: std::unique_ptr<zmq::socket_t> subscriber;

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

      /// \brief Mutex to guarantee exclusive access to exit variable.
      private: std::mutex exitMutex;

      /// \brief When true, the service thread will finish.
      private: bool exit;

      zbeacon_t *beacon;
    };
  }
}
#endif
