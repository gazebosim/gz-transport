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
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TopicsInfo.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
NodePrivatePtr NodePrivate::GetInstance(bool _verbose)
{
  static NodePrivatePtr instance(new NodePrivate(_verbose));
  return instance;
}

//////////////////////////////////////////////////
NodePrivate::NodePrivate(bool _verbose)
  : verbose(_verbose),
    context(new zmq::context_t(1)),
    publisher(new zmq::socket_t(*context, ZMQ_PUB)),
    subscriber(new zmq::socket_t(*context, ZMQ_SUB)),
    control(new zmq::socket_t(*context, ZMQ_DEALER)),
    timeout(Timeout)
{
  char bindEndPoint[1024];

  // Initialize random seed.
  srand(time(nullptr));

  // My UUID.
  uuid_generate(this->guid);
  this->guidStr = GetGuidStr(this->guid);

  // Initialize my discovery service.
  this->discovery.reset(new Discovery(this->guid, false));

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
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "Error: " << ze.what() << std::endl;
     std::exit(EXIT_FAILURE);
  }

  if (this->verbose)
  {
    std::cout << "Current host address: " << this->hostAddr << std::endl;
    std::cout << "Bind at: [" << this->myAddress << "] for pub/sub\n";
    std::cout << "Bind at: [" << this->myControlAddress << "] for control\n";
    std::cout << "GUID: " << this->guidStr << std::endl;
  }

  // We don't want to exit yet.
  this->exitMutex.lock();
  this->exit = false;
  this->exitMutex.unlock();

  // Start the service thread.
  this->threadReception =
    new std::thread(&NodePrivate::RunReceptionService, this);

  // Set the callback to notify discovery updates (new connections).
  discovery->SetConnectionsCb(&NodePrivate::OnNewConnection, this);

  // Set the callback to notify discovery updates (new disconnections).
  discovery->SetDisconnectionsCb(&NodePrivate::OnNewDisconnection, this);
}

//////////////////////////////////////////////////
NodePrivate::~NodePrivate()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service thread before exit.
  this->threadReception->join();
}

//////////////////////////////////////////////////
void NodePrivate::RunReceptionService()
{
  while (!this->discovery->Interrupted())
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {*this->subscriber, 0, ZMQ_POLLIN, 0},
      {*this->control, 0, ZMQ_POLLIN, 0}
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

    //  If we got a reply, process it
    if (items[0].revents & ZMQ_POLLIN)
      this->RecvMsgUpdate();
    if (items[1].revents & ZMQ_POLLIN)
      this->RecvControlUpdate();

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }

  this->exit = true;
}

//////////////////////////////////////////////////
int NodePrivate::Publish(const std::string &_topic, const std::string &_data)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->mutex);

  if (this->topics.AdvertisedByMe(_topic))
  {
    zmq::message_t message;
    message.rebuild(_topic.size() + 1);
    memcpy(message.data(), _topic.c_str(), _topic.size() + 1);
    this->publisher->send(message, ZMQ_SNDMORE);

    message.rebuild(this->myAddress.size() + 1);
    memcpy(message.data(), this->myAddress.c_str(),
           this->myAddress.size() + 1);
    this->publisher->send(message, ZMQ_SNDMORE);

    message.rebuild(_data.size() + 1);
    memcpy(message.data(), _data.c_str(), _data.size() + 1);
    this->publisher->send(message, 0);

    return 0;
  }
  else
  {
    if (this->verbose)
      std::cerr << "\nNot published. (" << _topic << ") not advertised\n";
    return -1;
  }
}

//////////////////////////////////////////////////
void NodePrivate::RecvMsgUpdate()
{
  std::lock_guard<std::mutex> lock(this->mutex);

  zmq::message_t message(0);
  std::string topic;
  // std::string sender;
  std::string data;

  try
  {
    if (!this->subscriber->recv(&message, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->subscriber->recv(&message, 0))
      return;
    // sender = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->subscriber->recv(&message, 0))
      return;
    data = std::string(reinterpret_cast<char *>(message.data()));
  }
  catch(const zmq::error_t &_error)
  {
    std::cout << "Error: " << _error.what() << std::endl;
    return;
  }

  if (this->topics.Subscribed(topic))
  {
    // Execute the callback registered
    ISubscriptionHandler_M handlers;
    this->topics.GetSubscriptionHandlers(topic, handlers);
    for (auto handler : handlers)
    {
      ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;
      if (subscriptionHandlerPtr)
        // ToDo(caguero): Unserialize only once.
        subscriptionHandlerPtr->RunCallback(topic, data);
      else
        std::cerr << "Subscription handler is NULL" << std::endl;
    }
  }
  else
    std::cerr << "I am not subscribed to topic [" << topic << "]\n";
}

//////////////////////////////////////////////////
void NodePrivate::RecvControlUpdate()
{
  std::cout << "Control update" << std::endl;

  std::lock_guard<std::mutex> lock(this->mutex);

  zmq::message_t message(0);
  std::string topic;
  std::string procUuid;
  std::string nodeUuid;
  std::string data;

  try
  {
    if (!this->control->recv(&message, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->control->recv(&message, 0))
      return;
    procUuid = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->control->recv(&message, 0))
      return;
    nodeUuid = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->control->recv(&message, 0))
      return;
    data = std::string(reinterpret_cast<char *>(message.data()));
  }
  catch(const zmq::error_t &_error)
  {
    std::cout << "Error: " << _error.what() << std::endl;
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

    this->topics.AddRemoteSubscriber(topic, procUuid, nodeUuid);
  }
  else if (std::stoi(data) == EndConnection)
  {
    if (this->verbose)
    {
      std::cout << "Registering the end of a remote connection" << std::endl;
      std::cout << "\tProc UUID: " << procUuid << std::endl;
      std::cout << "\tNode UUID: [" << nodeUuid << "]\n";
    }

    this->topics.DelRemoteSubscriber(topic, procUuid, nodeUuid);
  }
}

//////////////////////////////////////////////////
void NodePrivate::OnNewConnection(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrlAddr,
  const std::string &_uuid)
{
  // Register the advertised address for the topic.
  std::cout << "Connection callback" << std::endl;
  std::cout << "Topic: " << _topic << std::endl;
  std::cout << "Addr: " << _addr << std::endl;
  std::cout << "Ctrl Addr: " << _ctrlAddr << std::endl;
  std::cout << "UUID: [" << _uuid << "]" << std::endl;

  this->topics.AddAdvAddress(_topic, _addr, _ctrlAddr, _uuid);

  // Check if we are interested in this topic.
  if (this->topics.Subscribed(_topic) &&
      !this->Connected(_addr) &&
      this->guidStr.compare(_uuid) != 0)
  {
    std::cout << "Connecting to a remote publisher" << std::endl;
    try
    {
      this->subscriber->connect(_addr.c_str());
      this->subscriber->setsockopt(ZMQ_SUBSCRIBE, _topic.data(), _topic.size());
      this->AddConnection(_uuid, _addr, _ctrlAddr);

      // Send a message to the publisher's control socket to notify it
      // about all my subscribers.
      zmq::socket_t socket(*this->context, ZMQ_DEALER);
      socket.connect(_ctrlAddr.c_str());

      if (this->verbose)
      {
        std::cout << "\t* Connected to [" << _addr << "] for data\n";
        std::cout << "\t* Connected to [" << _ctrlAddr << "] for control\n";
      }

      // Set ZMQ_LINGER to 0 means no linger period. Pending messages will
      // be discarded immediately when the socket is closed. That avoids
      // infinite waits if the publisher is disconnected.
      int lingerVal = 1000;
      socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

      ISubscriptionHandler_M handlers;
      this->topics.GetSubscriptionHandlers(_topic, handlers);
      for (auto handler : handlers)
      {
        std::cout << "Sending control message" << std::endl;
        std::string nodeUuid = handler.second->GetNodeUuid();

        zmq::message_t message;
        message.rebuild(_topic.size() + 1);
        memcpy(message.data(), _topic.c_str(), _topic.size() + 1);
        socket.send(message, ZMQ_SNDMORE);

        message.rebuild(this->guidStr.size() + 1);
        memcpy(message.data(), this->guidStr.c_str(), this->guidStr.size() + 1);
        socket.send(message, ZMQ_SNDMORE);

        message.rebuild(nodeUuid.size() + 1);
        memcpy(message.data(), nodeUuid.c_str(), nodeUuid.size() + 1);
        socket.send(message, ZMQ_SNDMORE);

        std::string data = std::to_string(NewConnection);
        message.rebuild(data.size() + 1);
        memcpy(message.data(), data.c_str(), data.size() + 1);
        socket.send(message, 0);
      }
    }
    catch(const zmq::error_t& ze)
    {
      std::cout << "Error connecting [" << ze.what() << "]\n";
    }
  }
}

//////////////////////////////////////////////////
void NodePrivate::OnNewDisconnection(const std::string &/*_topic*/,
  const std::string &/*_addr*/, const std::string &/*_ctrlAddr*/,
  const std::string &_uuid)
{
  std::cout << "New disconnection detected " << std::endl;
  std::cout << "\tUUID: " << _uuid << std::endl;

  // A remote subscriber[s] has been disconnected.
  this->topics.DelRemoteSubscriber("", _uuid, "");

  // A remote publisher[s] has been disconnected.
  this->topics.DelAdvAddress("", "", _uuid);

  if (this->connections.find(_uuid) == this->connections.end())
    return;

  // Disconnect the sockets.
  for (auto connection : this->connections[_uuid])
    this->subscriber->disconnect(connection.addr.c_str());

  this->DelConnection(_uuid, "");
}

//////////////////////////////////////////////////
bool NodePrivate::Connected(const std::string &_addr)
{
  for (auto proc : this->connections)
  {
    for (auto connection : proc.second)
    {
      if (connection.addr == _addr)
        return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////
void NodePrivate::AddConnection(const std::string &_uuid,
  const std::string &_addr, const std::string &_ctrl)
{
  if (this->connections.find(_uuid) == this->connections.end())
  {
    this->connections[_uuid] = {{_addr, _ctrl}};
    return;
  }

  auto &v = this->connections[_uuid];
  auto found = std::find_if(v.begin(), v.end(),
      [=](Address_t _addrInfo)
      {
        return _addrInfo.addr == _addr;
      });
  // If the address was not existing before, just add it.
  if (found == v.end())
  {
    v.push_back({_addr, _ctrl});
  }
}

//////////////////////////////////////////////////
void NodePrivate::DelConnection(const std::string &_uuid,
  const std::string &_addr)
{
  for (auto it = this->connections.begin(); it != this->connections.end();)
  {
    auto &v = it->second;
    v.erase(std::remove_if(v.begin(), v.end(), [=](Address_t _addrInfo)
    {
      return _addrInfo.addr == _addr;
    }), v.end());

    it->second = v;

    if (v.empty() || it->first == _uuid)
      this->connections.erase(it++);
    else
      ++it;
  }
}
