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

#ifdef _MSC_VER
# pragma warning(push, 0)
#endif
#include <zmq.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
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
    requester(new zmq::socket_t(*context, ZMQ_ROUTER)),
    responseReceiver(new zmq::socket_t(*context, ZMQ_ROUTER)),
    replier(new zmq::socket_t(*context, ZMQ_ROUTER)),
    timeout(Timeout),
    exit(false)
{
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
    this->hostAddr = this->discovery->HostAddr();

    // Publisher socket listening in a random port.
    std::string anyTcpEp = "tcp://" + this->hostAddr + ":*";

    int lingerVal = 0;
    this->publisher->setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));
    this->publisher->bind(anyTcpEp.c_str());
    size_t size = sizeof(bindEndPoint);
    this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myAddress = bindEndPoint;

    // Control socket listening in a random port.
    this->control->bind(anyTcpEp.c_str());
    this->control->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myControlAddress = bindEndPoint;

    // ResponseReceiver socket listening in a random port.
    std::string id = this->responseReceiverId.ToString();
    this->responseReceiver->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
    this->responseReceiver->bind(anyTcpEp.c_str());
    this->responseReceiver->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myRequesterAddress = bindEndPoint;

    // Replier socket listening in a random port.
    id = this->replierId.ToString();
    this->replier->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
    int RouteOn = 1;
    this->replier->setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));
    this->replier->setsockopt(ZMQ_ROUTER_MANDATORY, &RouteOn, sizeof(RouteOn));
    this->replier->bind(anyTcpEp.c_str());
    this->replier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myReplierAddress = bindEndPoint;

    this->requester->setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));
    this->requester->setsockopt(ZMQ_ROUTER_MANDATORY, &RouteOn,
      sizeof(RouteOn));
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
    std::cout << "Identity for receiving srv. requests: ["
              << this->replierId.ToString() << "]" << std::endl;
    std::cout << "Identity for receiving srv. responses: ["
              << this->responseReceiverId.ToString() << "]" << std::endl;
  }

  // Start the service thread.
  this->threadReception = std::thread(&NodeShared::RunReceptionTask, this);

#ifdef _WIN32
  this->threadReceptionExiting = false;
  this->threadReception.detach();
#endif

  // Set the callback to notify discovery updates (new topics).
  discovery->ConnectionsCb(&NodeShared::OnNewConnection, this);

  // Set the callback to notify discovery updates (invalid topics).
  discovery->DisconnectionsCb(&NodeShared::OnNewDisconnection, this);

  // Set the callback to notify svc discovery updates (new services).
  discovery->ConnectionsSrvCb(&NodeShared::OnNewSrvConnection, this);

  // Set the callback to notify svc discovery updates (invalid services).
  discovery->DisconnectionsSrvCb(&NodeShared::OnNewSrvDisconnection, this);

  // Start the discovery service.
  discovery->Start();
}

//////////////////////////////////////////////////
NodeShared::~NodeShared()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Don't join on Windows, because it can hang when this object
  // is destructed on process exit (e.g., when it's a global static).
  // I think that it's due to this bug:
  // https://connect.microsoft.com/VisualStudio/feedback/details/747145/std-thread-join-hangs-if-called-after-main-exits-when-using-vs2012-rc
#ifndef _WIN32
  // Wait for the service thread before exit.
  if (this->threadReception.joinable())
    this->threadReception.join();

  // We explicitly destroy the ZMQ socket before destroying the ZMQ context.
  publisher.reset();
  subscriber.reset();
  control.reset();
  requester.reset();
  responseReceiver.reset();
  replier.reset();
  delete this->context;
#else
  while (true)
  {
    std::lock_guard<std::mutex> lock(this->exitMutex);
    {
      if (this->threadReceptionExiting)
        break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  // We intentionally don't destroy the context in Windows.
  // For some reason, when MATLAB deallocates the MEX file makes the context
  // destructor to hang (probably waiting for ZMQ sockets to terminate).
  // ToDo: Fix it.
#endif
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
      {*this->responseReceiver, 0, ZMQ_POLLIN, 0}
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

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
#ifdef _WIN32
  std::lock_guard<std::mutex> lock(this->exitMutex);
  {
    this->threadReceptionExiting = true;
  }
#endif
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
    Publisher remoteNode(topic, "", "", procUuid, nodeUuid, Scope_t::All,
      "");
    this->remoteSubscribers.AddPublisher(remoteNode);
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
    this->remoteSubscribers.DelPublisherByNode(topic, procUuid, nodeUuid);
  }
}

//////////////////////////////////////////////////
void NodeShared::RecvSrvRequest()
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (verbose)
    std::cout << "Message received requesting a service call" << std::endl;

  zmq::message_t msg(0);
  std::string topic;
  std::string sender;
  std::string nodeUuid;
  std::string reqUuid;
  std::string req;
  std::string rep;
  std::string resultStr;
  std::string dstId;

  try
  {
    if (!this->replier->recv(&msg, 0))
      return;

    if (!this->replier->recv(&msg, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->replier->recv(&msg, 0))
      return;
    sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->replier->recv(&msg, 0))
      return;
    dstId = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

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
    std::cerr << "NodeShared::RecvSrvRequest() error parsing request: "
              << _error.what() << std::endl;
    return;
  }

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

    // I am still not connected to this address.
    if (std::find(this->srvConnections.begin(), this->srvConnections.end(),
          sender) == this->srvConnections.end())
    {
      this->replier->connect(sender.c_str());
      this->srvConnections.push_back(sender);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      if (this->verbose)
      {
        std::cout << "\t* Connected to [" << sender
                  << "] for sending a response" << std::endl;
      }
    }

    // Send the reply.
    try
    {
      zmq::message_t response;

      response.rebuild(dstId.size());
      memcpy(response.data(), dstId.data(), dstId.size());
      this->replier->send(response, ZMQ_SNDMORE);

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
    catch(const zmq::error_t &_error)
    {
      std::cerr << "NodeShared::RecvSrvRequest() error sending response: "
                << _error.what() << std::endl;
      return;
    }
  }
  // else
  //  std::cerr << "I do not have a service call registered for topic ["
  //            << topic << "]\n";
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
    if (!this->responseReceiver->recv(&msg, 0))
      return;

    if (!this->responseReceiver->recv(&msg, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->responseReceiver->recv(&msg, 0))
      return;
    nodeUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->responseReceiver->recv(&msg, 0))
      return;
    reqUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->responseReceiver->recv(&msg, 0))
      return;
    rep = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

    if (!this->responseReceiver->recv(&msg, 0))
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
  std::string responserId;
  Addresses_M addresses;
  this->discovery->SrvPublishers(_topic, addresses);
  if (addresses.empty())
    return;

  // Get the first responder.
  auto &v = addresses.begin()->second;
  responserAddr = v.at(0).Addr();
  responserId = v.at(0).SocketId();

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
      req.second->Requested(true);

      auto data = req.second->Serialize();
      auto nodeUuid = req.second->NodeUuid();
      auto reqUuid = req.second->HandlerUuid();

      try
      {
        zmq::message_t msg;

        msg.rebuild(responserId.size());
        memcpy(msg.data(), responserId.data(), responserId.size());
        this->requester->send(msg, ZMQ_SNDMORE);

        msg.rebuild(_topic.size());
        memcpy(msg.data(), _topic.data(), _topic.size());
        this->requester->send(msg, ZMQ_SNDMORE);

        msg.rebuild(this->myRequesterAddress.size());
        memcpy(msg.data(), this->myRequesterAddress.data(),
          this->myRequesterAddress.size());
        this->requester->send(msg, ZMQ_SNDMORE);

        std::string myId = this->responseReceiverId.ToString();
        msg.rebuild(myId.size());
        memcpy(msg.data(), myId.data(), myId.size());
        this->requester->send(msg, ZMQ_SNDMORE);

        msg.rebuild(nodeUuid.size());
        memcpy(msg.data(), nodeUuid.data(), nodeUuid.size());
        this->requester->send(msg, ZMQ_SNDMORE);

        msg.rebuild(reqUuid.size());
        memcpy(msg.data(), reqUuid.data(), reqUuid.size());
        this->requester->send(msg, ZMQ_SNDMORE);

        msg.rebuild(data.size());
        memcpy(msg.data(), data.data(), data.size());
        this->requester->send(msg, 0);
      }
      catch(const zmq::error_t& ze)
      {
        // Debug output.
        // std::cerr << "Error connecting [" << ze.what() << "]\n";
      }
    }
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewConnection(const Publisher &_pub)
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  std::string topic = _pub.Topic();
  std::string addr = _pub.Addr();
  std::string ctrl = _pub.Ctrl();
  std::string procUuid = _pub.PUuid();

  if (this->verbose)
  {
    std::cout << "Connection callback" << std::endl;
    std::cout << _pub;
  }

  // Check if we are interested in this topic.
  if (this->localSubscriptions.HasHandlersForTopic(topic) &&
      this->pUuid.compare(procUuid) != 0)
  {
    try
    {
      // I am not connected to the process.
      if (!this->connections.HasPublisher(addr))
        this->subscriber->connect(addr.c_str());

      // Add a new filter for the topic.
      this->subscriber->setsockopt(ZMQ_SUBSCRIBE, topic.data(), topic.size());

      // Register the new connection with the publisher.
      this->connections.AddPublisher(_pub);

      // Send a message to the publisher's control socket to notify it
      // about all my remoteSubscribers.
      zmq::socket_t socket(*this->context, ZMQ_DEALER);

      if (this->verbose)
      {
        std::cout << "\t* Connected to [" << addr << "] for data\n";
        std::cout << "\t* Connected to [" << ctrl << "] for control\n";
      }

      int lingerVal = 300;
      socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));
      socket.connect(ctrl.c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(300));

      std::map<std::string, ISubscriptionHandler_M> handlers;
      if (this->localSubscriptions.GetHandlers(topic, handlers))
      {
        for (auto &node : handlers)
        {
          for (auto &handler : node.second)
          {
            std::string nodeUuid = handler.second->NodeUuid();

            zmq::message_t msg;
            msg.rebuild(topic.size());
            memcpy(msg.data(), topic.data(), topic.size());
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
void NodeShared::OnNewDisconnection(const Publisher &_pub)
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  std::string topic = _pub.Topic();
  std::string procUuid = _pub.PUuid();
  std::string nUuid = _pub.NUuid();

  if (this->verbose)
  {
    std::cout << "New disconnection detected " << std::endl;
    std::cout << "\tProcess UUID: " << procUuid << std::endl;
  }

  // A remote subscriber[s] has been disconnected.
  if (topic != "" && nUuid != "")
  {
    this->remoteSubscribers.DelPublisherByNode(topic, procUuid, nUuid);

    Publisher connection;
    if (!this->connections.GetPublisher(topic, procUuid, nUuid, connection))
      return;

    // Disconnect from a publisher's socket.
    // for (const auto &connection : this->connections[procUuid])
    //   this->subscriber->disconnect(connection.addr.c_str());
    this->subscriber->disconnect(connection.Addr().c_str());

    // I am no longer connected.
    this->connections.DelPublisherByNode(topic, procUuid, nUuid);
  }
  else
  {
    this->remoteSubscribers.DelPublishersByProc(procUuid);

    Addresses_M info;
    if (!this->connections.GetPublishers(topic, info))
      return;

    // Disconnect from all the connections of that publisher.
    for (auto &connection : info[procUuid])
      this->subscriber->disconnect(connection.Addr().c_str());

    // Remove all the connections from the process disonnected.
    this->connections.DelPublishersByProc(procUuid);
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewSrvConnection(const Publisher &_pub)
{
  std::string topic = _pub.Topic();
  std::string addr = _pub.Addr();

  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (this->verbose)
  {
    std::cout << "Service call connection callback" << std::endl;
    std::cout << _pub;
  }

  // I am still not connected to this address.
  if (std::find(this->srvConnections.begin(), this->srvConnections.end(),
        addr) == this->srvConnections.end())
  {
    this->requester->connect(addr.c_str());
    this->srvConnections.push_back(addr);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (this->verbose)
    {
      std::cout << "\t* Connected to [" << addr
                << "] for service requests" << std::endl;
    }
  }

  // Request all pending service calls for this topic.
  this->SendPendingRemoteReqs(topic);
}

//////////////////////////////////////////////////
void NodeShared::OnNewSrvDisconnection(const Publisher &_pub)
{
  std::string addr = _pub.Addr();

  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  // Remove the address from the list of connected addresses.
  this->srvConnections.erase(std::remove(std::begin(this->srvConnections),
    std::end(this->srvConnections), addr.c_str()),
    std::end(this->srvConnections));

  if (this->verbose)
  {
    std::cout << "Service call disconnection callback" << std::endl;
    std::cout << _pub;
  }
}
