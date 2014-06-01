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
#include <uuid/uuid.h>
#include <zmq.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/TopicsInfo.hh"

namespace ignition
{
  namespace transport
  {
    /// \class NodePrivate NodePrivate.hh
    /// \brief Private data for the Node class.
    class NodePrivate
    {
      /// \brief NodePrivate is a singleton. This method gets the
      /// NodePrivate instance shared between all the nodes.
      /// \param[in] _verbose True if you want to see debug messages.
      /// \return NodePrivatePtr Pointer to the current NodePrivate instance.
      public: static NodePrivatePtr GetInstance(bool _verbose);

      /// \brief Constructor.
      /// \param[in] _verbose true for enabling verbose mode.
      public: NodePrivate(bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~NodePrivate();

      /// \brief Receive data and control messages.
      public: void RunReceptionService();

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

      public: void OnNewConnection(const std::string &_topic,
        const std::string &_addr, const std::string &_ctrlAddr,
        const std::string &_uuid);

      public: void OnNewDisconnection(const std::string &_topic,
        const std::string &_addr, const std::string &_ctrlAddr,
        const std::string &_uuid);

      public: bool Connected(const std::string &_addr);
      public: void AddConnection(const std::string &_uuid,
        const std::string &_addr, const std::string &_ctrl);
      public: void DelConnection(const std::string &_uuid,
        const std::string &_addr);

      /// \brief Timeout used for receiving messages.
      public: static const int Timeout = 250;

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

      /// \brief Discovery service.
      public: std::unique_ptr<Discovery> discovery;

      /// \brief 0MQ context.
      public: std::unique_ptr<zmq::context_t> context;

      /// \brief ZMQ socket to send topic updates.
      public: std::unique_ptr<zmq::socket_t> publisher;

      /// \brief ZMQ socket to receive topic updates.
      public: std::shared_ptr<zmq::socket_t> subscriber;

      /// \brief ZMQ socket to receive control updates (new connections, ...).
      public: std::unique_ptr<zmq::socket_t> control;

      /// \brief Local GUID.
      public: uuid_t guid;

      /// \brief String conversion of the GUID.
      public: std::string guidStr;

      /// \brief Timeout used for receiving requests.
      public: int timeout;

      /// \brief thread in charge of receiving and handling incoming messages.
      public: std::thread *threadReception;

      /// \brief Mutex to guarantee exclusive access between the inbound and
      /// outbound thread.
      public: std::mutex mutex;

      /// \brief Mutex to guarantee exclusive access to exit variable.
      private: std::mutex exitMutex;

      /// \brief When true, the service thread will finish.
      private: bool exit;

      private: Addresses_M connections;
    };
  }
}
#endif
