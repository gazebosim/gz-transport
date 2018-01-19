/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <zmq.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>

#include "WorkerPool.hh"
#include "ignition/transport/Discovery.hh"

using namespace ignition;
using namespace transport;

// Private data class for NodeShared.
class ignition::transport::NodeSharedPrivate
{
  // Constructor
  public: NodeSharedPrivate() :
            context(new zmq::context_t(1)),
            publisher(new zmq::socket_t(*context, ZMQ_PUB)),
            subscriber(new zmq::socket_t(*context, ZMQ_SUB)),
            control(new zmq::socket_t(*context, ZMQ_DEALER)),
            requester(new zmq::socket_t(*context, ZMQ_ROUTER)),
            responseReceiver(new zmq::socket_t(*context, ZMQ_ROUTER)),
            replier(new zmq::socket_t(*context, ZMQ_ROUTER))
  {
  }

  /// A thread pool
  public: WorkerPool workerPool;

  /// \brief Initialize security
  public: void SecurityInit();

  /// \brief Handle new secure connections
  public: void SecurityOnNewConnection();

  /// \brief Access control handler for plain security.
  /// This function is designed to be run in a thread.
  public: void AccessControlHandler();

  //////////////////////////////////////////////////
  ///////    Declare here the ZMQ Context    ///////
  //////////////////////////////////////////////////

  /// \brief 0MQ context. Always declare this object before any ZMQ socket
  /// to make sure that the context is destroyed after all sockets.
  public: std::unique_ptr<zmq::context_t> context;

  //////////////////////////////////////////////////
  ///////     Declare here all ZMQ sockets   ///////
  //////////////////////////////////////////////////

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

  /// \brief ZMQ socket to receive service call requests.
  public: std::unique_ptr<zmq::socket_t> replier;

  /// \brief Thread the handle access control
  public: std::thread accessControlThread;

  //////////////////////////////////////////////////
  /////// Declare here the discovery object  ///////
  //////////////////////////////////////////////////

  /// \brief Discovery service (messages).
  public: std::unique_ptr<MsgDiscovery> msgDiscovery;

  /// \brief Discovery service (services).
  public: std::unique_ptr<SrvDiscovery> srvDiscovery;

  //////////////////////////////////////////////////
  /////// Other private member variables     ///////
  //////////////////////////////////////////////////

  /// \brief When true, the reception thread will finish.
  public: bool exit = false;

  /// \brief Mutex to guarantee exclusive access to the 'exit' variable.
  public: std::mutex exitMutex;

  /// \brief Timeout used for receiving messages (ms.).
  public: static const int Timeout = 250;
};
