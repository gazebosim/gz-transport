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

#include <uuid/uuid.h>
#include <zmq.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
NodeShared *NodeShared::GetInstance()
{
  static NodeShared instance;
  return &instance;
}

//////////////////////////////////////////////////
NodeShared::NodeShared()
  : verbose(false),
    context(new zmq::context_t(1)),
    publisher(new zmq::socket_t(*context, ZMQ_PUB)),
    subscriber(new zmq::socket_t(*context, ZMQ_SUB)),
    control(new zmq::socket_t(*context, ZMQ_DEALER)),
    requester(new zmq::socket_t(*context, ZMQ_XREQ)),
    replier(new zmq::socket_t(*context, ZMQ_XREP)),
    timeout(Timeout),
    exit(false)
{
  //zmq_ctx_set(this->context.get(), ZMQ_MAX_SOCKETS, 1024);
  //int max_sockets = zmq_ctx_get(this->context.get(), ZMQ_MAX_SOCKETS);
  //std::cout << "Max sockets: " << max_sockets << std::endl;
  //assert (max_sockets == 1024);

  // If IGN_VERBOSE=1 enable the verbose mode.
  char const *tmp = std::getenv("IGN_VERBOSE");
  if (tmp)
    this->verbose = std::string(tmp) == "1";

  char bindEndPoint[1024];

  // My process UUID.
  Uuid uuid;
  this->pUuid = uuid.ToString();

  // Initialize my discovery service.
  this->discovery.reset(new Discovery(this->pUuid, false));

  // Initialize the 0MQ objects.
  try
  {
    // Set the hostname's ip address.
    this->hostAddr = this->discovery->GetHostAddr();

    // Publisher socket listening in a random port.
    std::string anyTcpEp = "tcp://" + this->hostAddr + ":*";
    this->publisher->bind(anyTcpEp.c_str());
    size_t size = sizeof(bindEndPoint);
    this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myAddress = bindEndPoint;

    // Control socket listening in a random port.
    this->control->bind(anyTcpEp.c_str());
    this->control->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myControlAddress = bindEndPoint;

    // Requester socket listening in a random port.
    this->requester->bind(anyTcpEp.c_str());
    this->requester->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myRequesterAddress = bindEndPoint;

    // Replier socket listening in a random port.
    this->replier->bind(anyTcpEp.c_str());
    this->replier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myReplierAddress = bindEndPoint;
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "NodeShared() Error: " << ze.what() << std::endl;
     std::exit(EXIT_FAILURE);
  }

  if (this->verbose)
  {
    std::cout << "Current host address: " << this->hostAddr << std::endl;
    std::cout << "Process UUID: " << this->pUuid << std::endl;
    std::cout << "Bind at: [" << this->myAddress << "] for pub/sub\n";
    std::cout << "Bind at: [" << this->myControlAddress << "] for control\n";
    std::cout << "Bind at: [" << this->myReplierAddress << "] for srv. calls\n";
  }

  // Start the service thread.
  this->threadReception = new std::thread(&NodeShared::RunReceptionTask, this);

  // Set the callback to notify discovery updates (new topics).
  discovery->SetConnectionsCb(&NodeShared::OnNewConnection, this);

  // Set the callback to notify discovery updates (invalid topics).
  discovery->SetDisconnectionsCb(&NodeShared::OnNewDisconnection, this);

  // Set the callback to notify svc discovery updates (new service calls).
  discovery->SetConnectionsSrvCb(&NodeShared::OnNewSrvConnection, this);
}

//////////////////////////////////////////////////
NodeShared::~NodeShared()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service thread before exit.
  this->threadReception->join();
}

//////////////////////////////////////////////////
void NodeShared::RunReceptionTask()
{
  while (true)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {*this->subscriber, 0, ZMQ_POLLIN, 0},
      {*this->control, 0, ZMQ_POLLIN, 0},
      {*this->replier, 0, ZMQ_POLLIN, 0},
      {*this->requester, 0, ZMQ_POLLIN, 0}
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

    std::cout << "Poll exits" << std::endl;

    //  If we got a reply, process it.
    if (items[0].revents & ZMQ_POLLIN)
      this->RecvMsgUpdate();
    if (items[1].revents & ZMQ_POLLIN)
      this->RecvControlUpdate();
    if (items[2].revents & ZMQ_POLLIN)
      this->RecvSrvRequest();
    if (items[3].revents & ZMQ_POLLIN)
      this->RecvSrvResponse();

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
bool NodeShared::Publish(const std::string &_topic, const std::string &_data)
{
  try
  {
    zmq::message_t msg;
    msg.rebuild(_topic.size());
    memcpy(msg.data(), _topic.data(), _topic.size());
    this->publisher->send(msg, ZMQ_SNDMORE);

    msg.rebuild(this->myAddress.size());
    memcpy(msg.data(), this->myAddress.data(), this->myAddress.size());
    this->publisher->send(msg, ZMQ_SNDMORE);

    msg.rebuild(_data.size());
    memcpy(msg.data(), _data.data(), _data.size());
    this->publisher->send(msg, 0);
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "NodeShared::Publish() Error: " << ze.what() << std::endl;
     return false;
  }


  return true;
}

//////////////////////////////////////////////////
void NodeShared::RecvMsgUpdate()
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  zmq::message_t msg(0);
  std::string topic;
  // std::string sender;
  std::string data;

  try
  {
    if (!this->subscriber->recv(&msg, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    // ToDo(caguero): Use this as extra metadata for the subscriber.
    if (!this->subscriber->recv(&msg, 0))
      return;
    // sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->subscriber->recv(&msg, 0))
      return;
    data = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
  }
  catch(const zmq::error_t &_error)
  {
    std::cout << "Error: " << _error.what() << std::endl;
    return;
  }


  // Execute the callbacks registered.
  std::map<std::string, ISubscriptionHandler_M> handlers;
  if (this->localSubscriptions.GetHandlers(topic, handlers))
  {
    for (auto &node : handlers)
    {
      for (auto &handler : node.second)
      {
        ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;
        if (subscriptionHandlerPtr)
        {
          // ToDo(caguero): Unserialize only once.
          subscriptionHandlerPtr->RunCallback(topic, data);
        }
        else
          std::cerr << "Subscription handler is NULL" << std::endl;
      }
    }
  }
  else
    std::cerr << "I am not subscribed to topic [" << topic << "]\n";
}

//////////////////////////////////////////////////
void NodeShared::RecvControlUpdate()
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  zmq::message_t msg(0);
  std::string topic;
  std::string procUuid;
  std::string nodeUuid;
  std::string data;

  try
  {
    if (!this->control->recv(&msg, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->control->recv(&msg, 0))
      return;
    procUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->control->recv(&msg, 0))
      return;
    nodeUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->control->recv(&msg, 0))
      return;
    data = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
  }
  catch(const zmq::error_t &_error)
  {
    std::cerr << "NodeShared::RecvControlUpdate() error: "
              << _error.what() << std::endl;
    return;
  }

  if (std::stoi(data) == NewConnection)
  {
    if (this->verbose)
    {
      std::cout << "Registering a new remote connection" << std::endl;
      std::cout << "\tProc UUID: [" << procUuid << "]\n";
      std::cout << "\tNode UUID: [" << nodeUuid << "]\n";
    }

    // Register that we have another remote subscriber.
    this->remoteSubscribers.AddAddress(topic, "", "", procUuid, nodeUuid);
  }
  else if (std::stoi(data) == EndConnection)
  {
    if (this->verbose)
    {
      std::cout << "Registering the end of a remote connection" << std::endl;
      std::cout << "\tProc UUID: " << procUuid << std::endl;
      std::cout << "\tNode UUID: [" << nodeUuid << "]\n";
    }

    // Delete a remote subscriber.
    this->remoteSubscribers.DelAddressByNode(topic, procUuid, nodeUuid);
  }
}

//////////////////////////////////////////////////
void NodeShared::RecvSrvRequest()
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  //if (verbose)
    std::cout << "Message received requesting a service call" << std::endl;

  zmq::message_t msg(0);
  std::string topic;
  std::string sender;
  std::string nodeUuid;
  std::string reqUuid;
  std::string req;
  std::string rep;
  std::string resultStr;

  try
  {
    if (!this->replier->recv(&msg, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->replier->recv(&msg, 0))
      return;
    sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->replier->recv(&msg, 0))
      return;
    nodeUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->replier->recv(&msg, 0))
      return;
    reqUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->replier->recv(&msg, 0))
      return;
    req = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
  }
  catch(const zmq::error_t &_error)
  {
    std::cerr << "NodeShared::RecvSrvRequest() error: "
              << _error.what() << std::endl;
    return;
  }

  std::cout << "Req received: " << reqUuid << std::endl;

  // Get the REP handler.
  IRepHandlerPtr repHandler;
  if (this->repliers.GetHandler(topic, repHandler))
  {
    bool result;
    // Run the service call and get the results.
    repHandler->RunCallback(topic, req, rep, result);

    if (result)
      resultStr = "1";
    else
      resultStr = "0";

    // Send the reply.
    //zmq::context_t ctx;
    //zmq::socket_t socket(ctx, ZMQ_DEALER);
    //socket.connect(sender.c_str());

    // Set ZMQ_LINGER to 0 means no linger period. Pending messages will
    // be discarded immediately when the socket is closed. That avoids
    // infinite waits if the publisher is disconnected.
    //int lingerVal = 200;
    //socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

    zmq::message_t response;
    response.rebuild(topic.size());
    memcpy(response.data(), topic.data(), topic.size());
    this->replier->send(response, ZMQ_SNDMORE);

    response.rebuild(nodeUuid.size());
    memcpy(response.data(), nodeUuid.data(), nodeUuid.size());
    this->replier->send(response, ZMQ_SNDMORE);

    response.rebuild(reqUuid.size());
    memcpy(response.data(), reqUuid.data(), reqUuid.size());
    this->replier->send(response, ZMQ_SNDMORE);

    response.rebuild(rep.size());
    memcpy(response.data(), rep.data(), rep.size());
    this->replier->send(response, ZMQ_SNDMORE);

    response.rebuild(resultStr.size());
    memcpy(response.data(), resultStr.data(), resultStr.size());
    this->replier->send(response, 0);
  }
  else
    std::cerr << "I do not have a service call registered for topic ["
              << topic << "]\n";
}

//////////////////////////////////////////////////
void NodeShared::RecvSrvResponse()
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (verbose)
    std::cout << "Message received containing a service call REP" << std::endl;

  zmq::message_t msg(0);
  std::string topic;
  std::string nodeUuid;
  std::string reqUuid;
  std::string rep;
  std::string resultStr;
  bool result;

  try
  {
    if (!this->requester->recv(&msg, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->requester->recv(&msg, 0))
      return;
    nodeUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->requester->recv(&msg, 0))
      return;
    reqUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->requester->recv(&msg, 0))
      return;
    rep = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->requester->recv(&msg, 0))
      return;
    resultStr = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
    result = resultStr == "1";
  }
  catch(const zmq::error_t &_error)
  {
    std::cerr << "NodeShared::RecvSrvResponse() error: "
              << _error.what() << std::endl;
    return;
  }

  IReqHandlerPtr reqHandlerPtr;
  if (this->requests.GetHandler(topic, nodeUuid, reqUuid, reqHandlerPtr))
  {
    // Notify the result.
    reqHandlerPtr->NotifyResult(topic, rep, result);

    // Remove the handler.
    this->requests.RemoveHandler(topic, nodeUuid, reqUuid);
  }
  else
  {
    std::cerr << "Received a service call response but I don't have a handler"
              << " for it" << std::endl;
  }
}

//////////////////////////////////////////////////
void NodeShared::SendPendingRemoteReqs(const std::string &_topic)
{
  std::string responserAddr;
  Addresses_M addresses;
  this->discovery->GetSrvAddresses(_topic, addresses);
  if (addresses.empty())
    return;

  // Get the first responder.
  auto &v = addresses.begin()->second;
  responserAddr = v.at(0).addr;

  if (verbose)
  {
    std::cout << "Found a service call responser at ["
              << responserAddr << "]" << std::endl;
  }

  // Send all the pending REQs.
  IReqHandler_M reqs;
  if (!this->requests.GetHandlers(_topic, reqs))
    return;

  for (auto &node : reqs)
  {
    for (auto &req : node.second)
    {
      // Check if this service call has been already requested.
      if (req.second->Requested())
        continue;

      // Mark the handler as requested.
      req.second->SetRequested(true);

      auto data = req.second->Serialize();
      auto nodeUuid = req.second->GetNodeUuid();
      auto reqUuid = req.second->GetHandlerUuid();

      try
      {
        static int counter = 0;
        //zmq::context_t ctx;
        //zmq::socket_t socket(ctx, ZMQ_DEALER);
        std::cout << "Num sockets: " << ++counter << std::endl;
        this->requester->connect(responserAddr.c_str());

        // Set ZMQ_LINGER to 0 means no linger period. Pending messages will
        // be discarded immediately when the socket is closed. That avoids
        // infinite waits if the publisher is disconnected.
        //int lingerVal = 200;
        //socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

        zmq::message_t msg;
        msg.rebuild(_topic.size());
        memcpy(msg.data(), _topic.data(), _topic.size());
        assert(this->requester->send(msg, ZMQ_SNDMORE) > 0);

        msg.rebuild(this->myRequesterAddress.size());
        memcpy(msg.data(), this->myRequesterAddress.data(),
          this->myRequesterAddress.size());
        assert(this->requester->send(msg, ZMQ_SNDMORE) > 0);

        msg.rebuild(nodeUuid.size());
        memcpy(msg.data(), nodeUuid.data(), nodeUuid.size());
        assert(this->requester->send(msg, ZMQ_SNDMORE) > 0);

        msg.rebuild(reqUuid.size());
        memcpy(msg.data(), reqUuid.data(), reqUuid.size());
        assert(this->requester->send(msg, ZMQ_SNDMORE) > 0);

        msg.rebuild(data.size());
        memcpy(msg.data(), data.data(), data.size());
        assert(this->requester->send(msg, 0) > 0);

        std::cout << "Sending " << reqUuid << std::endl;
      }
      catch(const zmq::error_t& ze)
      {
        std::cerr << "Error connecting [" << ze.what() << "]\n";
      }
    }
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewConnection(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl,
  const std::string &_pUuid, const std::string &_nUuid,
  const Scope &_scope)
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (this->verbose)
  {
    std::cout << "Connection callback" << std::endl;
    std::cout << "Topic: " << _topic << std::endl;
    std::cout << "Addr: " << _addr << std::endl;
    std::cout << "Ctrl Addr: " << _ctrl << std::endl;
    std::cout << "Process UUID: [" << _pUuid << "]" << std::endl;
    std::cout << "Node UUID: [" << _nUuid << "]" << std::endl;
  }

  // Check if we are interested in this topic.
  if (this->localSubscriptions.HasHandlersForTopic(_topic) &&
      this->pUuid.compare(_pUuid) != 0)
  {
    try
    {
      // I am not connected to the process.
      if (!this->connections.HasAddress(_addr))
        this->subscriber->connect(_addr.c_str());

      // Add a new filter for the topic.
      this->subscriber->setsockopt(ZMQ_SUBSCRIBE, _topic.data(), _topic.size());

      // Register the new connection with the publisher.
      this->connections.AddAddress(
        _topic, _addr, _ctrl, _pUuid, _nUuid, _scope);

      // Send a message to the publisher's control socket to notify it
      // about all my remoteSubscribers.
      zmq::context_t ctx;
      zmq::socket_t socket(ctx, ZMQ_DEALER);
      socket.connect(_ctrl.c_str());

      if (this->verbose)
      {
        std::cout << "\t* Connected to [" << _addr << "] for data\n";
        std::cout << "\t* Connected to [" << _ctrl << "] for control\n";
      }

      // Set ZMQ_LINGER to 0 means no linger period. Pending messages will
      // be discarded immediately when the socket is closed. That avoids
      // infinite waits if the publisher is disconnected.
      int lingerVal = 0;
      socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

      std::map<std::string, ISubscriptionHandler_M> handlers;
      if (this->localSubscriptions.GetHandlers(_topic, handlers))
      {
        for (auto &node : handlers)
        {
          for (auto &handler : node.second)
          {
            std::string nodeUuid = handler.second->GetNodeUuid();

            zmq::message_t msg;
            msg.rebuild(_topic.size());
            memcpy(msg.data(), _topic.data(), _topic.size());
            socket.send(msg, ZMQ_SNDMORE);

            msg.rebuild(this->pUuid.size());
            memcpy(msg.data(), this->pUuid.data(), this->pUuid.size());
            socket.send(msg, ZMQ_SNDMORE);

            msg.rebuild(nodeUuid.size());
            memcpy(msg.data(), nodeUuid.data(), nodeUuid.size());
            socket.send(msg, ZMQ_SNDMORE);

            std::string data = std::to_string(NewConnection);
            msg.rebuild(data.size());
            memcpy(msg.data(), data.data(), data.size());
            socket.send(msg, 0);
          }
        }
      }
    }
    // The remote node might not be available when we are connecting.
    catch(const zmq::error_t& ze)
    {
    }
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewDisconnection(const std::string &_topic,
  const std::string &/*_addr*/, const std::string &/*_ctrlAddr*/,
  const std::string &_pUuid, const std::string &_nUuid,
  const Scope &/*_scope*/)
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (this->verbose)
  {
    std::cout << "New disconnection detected " << std::endl;
    std::cout << "\tProcess UUID: " << _pUuid << std::endl;
  }

  // A remote subscriber[s] has been disconnected.
  if (_topic != "" && _nUuid != "")
  {
    this->remoteSubscribers.DelAddressByNode(_topic, _pUuid, _nUuid);

    Address_t connection;
    if (!this->connections.GetAddress(_topic, _pUuid, _nUuid, connection))
      return;

    // Disconnect from a publisher's socket.
    // for (const auto &connection : this->connections[_pUuid])
    //   this->subscriber->disconnect(connection.addr.c_str());
    this->subscriber->disconnect(connection.addr.c_str());

    // I am no longer connected.
    this->connections.DelAddressByNode(_topic, _pUuid, _nUuid);
  }
  else
  {
    this->remoteSubscribers.DelAddressesByProc(_pUuid);

    Addresses_M info;
    if (!this->connections.GetAddresses(_topic, info))
      return;

    // Disconnect from all the connections of that publisher.
    for (auto &connection : info[_pUuid])
      this->subscriber->disconnect(connection.addr.c_str());

    // Remove all the connections from the process disonnected.
    this->connections.DelAddressesByProc(_pUuid);
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewSrvConnection(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl,
  const std::string &_pUuid, const std::string &_nUuid,
  const Scope &/*_scope*/)
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (this->verbose)
  {
    std::cout << "Service call connection callback" << std::endl;
    std::cout << "Topic: " << _topic << std::endl;
    std::cout << "Addr: " << _addr << std::endl;
    std::cout << "Ctrl Addr: " << _ctrl << std::endl;
    std::cout << "Process UUID: [" << _pUuid << "]" << std::endl;
    std::cout << "Node UUID: [" << _nUuid << "]" << std::endl;
  }

  // Request all pending service calls for this topic.
  this->SendPendingRemoteReqs(_topic);
}
