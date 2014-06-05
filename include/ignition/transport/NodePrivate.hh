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
      public: void RunReceptionTask();

      /// \brief Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _data Data to publish.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic,
                          const std::string &_data);

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdate();

      /// \brief Method in charge of receiving the topic updates.
      public: void RecvMsgUpdate();

      /// \brief Method in charge of receiving the control updates (new remote
      /// subscriber for example).
      public: void RecvControlUpdate();

      /// \brief Callback executed when the discovery detects new connections.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ address of the publisher.
      /// \param[in] _ctrl 0MQ control address of the publisher.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[in] _scope Topic scope.
      public: void OnNewConnection(const std::string &_topic,
                                   const std::string &_addr,
                                   const std::string &_ctrl,
                                   const std::string &_pUuid,
                                   const std::string &_nUuid,
                                   const Scope &_scope);

      /// \brief Callback executed when the discovery detects disconnections.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ address of the publisher.
      /// \param[in] _ctrl 0MQ control address of the publisher.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[in] _scope Topic scope.
      public: void OnNewDisconnection(const std::string &_topic,
                                      const std::string &_addr,
                                      const std::string &_ctrl,
                                      const std::string &_pUuid,
                                      const std::string &_nUuid,
                                      const Scope &_scope);

      /// \brief Check if this node is connected to a remote 0MQ address.
      /// \param[in] _addr 0MQ address to check.
      /// \return true when this node is remotely connected to the address.
      public: bool Connected(const std::string &_addr);

      /// \brief Register a new remote connection.
      /// \param[in] _topic Topic name
      /// \param[in] _addr 0MQ address of the publisher.
      /// \param[in] _ctrl 0MQ control address of the publisher.
      /// \param[in] _pUuid Publisher's process UUID.
      /// \param[in] _nUuid Publisher's node UUID.
      /// \param[in] _scope Topic scope.
      public: void AddConnection(const std::string &_topic,
                                 const std::string &_addr,
                                 const std::string &_ctrl,
                                 const std::string &_pUuid,
                                 const std::string &_nUuid,
                                 const Scope &_scope);

      /// \brief Get information about a node that I am connected.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's UUID.
      /// \param[out] _info Information requested.
      /// \return true when the connection requested is found.
      public: bool GetConnection(const std::string &_topic,
                                 const std::string &_nUuid,
                                 Address_t &_info);

      /// \brief Remove a new remote connection.
      /// \param[in] _topic Topic name.
      /// \param[in] _uuid Process UUID of the publisher.
      /// \param[in] _nUuid Node's UUID.
      public: void DelConnectionByNode(const std::string &_topic,
                                       const std::string &_pUuid,
                                       const std::string &_nUuid);

      /// \brief Remove all the remote connections against a process pUuid.
      /// \param[in] _uuid Process UUID of the publisher.
      public: void DelConnectionByProc(const std::string &_pUuid);

      /// \brief Timeout used for receiving messages (ms.).
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
      public: std::thread *threadReception;

      /// \brief Mutex to guarantee exclusive access between the inbound and
      /// outbound thread.
      public: std::recursive_mutex mutex;

      /// \brief When true, the service thread will finish.
      public: bool exit;

      /// \brief Mutex to guarantee exclusive access to exit variable.
      private: std::mutex exitMutex;

      /// \brief Remote connections.
      private: std::map<std::string, Addresses_M> connections;
    };
  }
}
#endif
