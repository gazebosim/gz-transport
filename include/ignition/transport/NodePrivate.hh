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

#ifndef _IGN_TRANSPORT_NODEPRIVATE_HH_INCLUDED__
#define _IGN_TRANSPORT_NODEPRIVATE_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <czmq.h>
#include <uuid/uuid.h>
#include <zmq.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ignition/transport/TopicsInfo.hh"

namespace ignition
{
  namespace transport
  {
    /// \class NodePrivate NodePrivate.hh
    /// \brief Private data for the Node class.
    class NodePrivate
    {
      /// \brief Port used to broadcast the discovery messages.
      public: static const int DiscoveryPort = 11312;

      /// \brief Broadcast interval for discovery beacons in milliseconds.
      public: static const int BeaconInterval = 2500;

      /// \brief NodePrivate is a singleton. This method gets the
      /// NodePrivate instance shared between all the nodes.
      /// \param[in] _verbose True if you want to see debug messages.
      /// \return NodePrivatePtr Pointer to the current NodePrivate instance.
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

      /// \brief Method in charge of receiving the control updates (new remote
      /// subscriber).
      public: void RecvControlUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      private: int DispatchDiscoveryMsg(char *_msg);

      /// \brief Send an ADVERTISE message to the discovery socket.
      /// \param[in] _type ADV or ADV_SVC.
      /// \param[in] _topic Topic to be advertised.
      /// \return 0 when success.
      public: int SendAdvertiseMsg(uint8_t _type, const std::string &_topic);

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
      public: std::string myAddress;

      /// \brief My pub/sub control address.
      public: std::string myControlAddress;

      /// \brief IP address of this host.
      public: std::string hostAddr;

      /// \brief UDP broadcast port used for the transport.
      public: int bcastPort;

      /// \brief 0MQ context.
      public: std::unique_ptr<zmq::context_t> context;

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisher;

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisherLocal;

      /// \brief ZMQ socket to receive topic updates.
      public: std::unique_ptr<zmq::socket_t> subscriber;

      /// \brief ZMQ socket to receive control updates (new connections, ...).
      public: std::unique_ptr<zmq::socket_t> control;

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

      /// \brief ZMQ context for the discovery beacon.
      public: zctx_t *ctx;

      /// \brief Mutex to guarantee exclusive access to exit variable.
      private: std::mutex exitMutex;

      /// \brief When true, the service thread will finish.
      private: bool exit;

      /// \brief Discovery beacon.
      private: zbeacon_t *beacon;
    };
  }
}
#endif
