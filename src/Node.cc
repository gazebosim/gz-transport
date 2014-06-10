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

#include <czmq.h>
#include <google/protobuf/message.h>
#include <algorithm>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/Node.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Node::Node(bool _verbose)
  : dataPtr(new NodePrivate())
{
  uuid_generate(this->dataPtr->nUuid);
  this->dataPtr->nUuidStr = GetGuidStr(this->dataPtr->nUuid);
  this->dataPtr->shared = NodeShared::GetInstance(_verbose);
  this->dataPtr->verbose = _verbose;
}

//////////////////////////////////////////////////
Node::~Node()
{
  // Unsubscribe from all the topics.
  for (auto topic : this->dataPtr->topicsSubscribed)
    this->Unsubscribe(topic);

  // Unadvertise all my topics.
  while (!this->dataPtr->topicsAdvertised.empty())
  {
    auto topic = *this->dataPtr->topicsAdvertised.begin();
    this->Unadvertise(topic);
  }

  // Unadvertise all my service calls.
  for (auto topic : this->dataPtr->srvsAdvertised)
    this->UnadvertiseSrv(topic);
}

//////////////////////////////////////////////////
void Node::Advertise(const std::string &_topic, const Scope &_scope)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Add the topic to the list of advertised topics (if it was not before)
  this->dataPtr->topicsAdvertised.insert(_topic);

  // Notify the discovery service to register and advertise my topic.
  this->dataPtr->shared->discovery->AdvertiseMsg(_topic,
    this->dataPtr->shared->myAddress, this->dataPtr->shared->myControlAddress,
    this->dataPtr->nUuidStr, _scope);
}

//////////////////////////////////////////////////
void Node::Unadvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::cout << "Unadvertise " << _topic << std::endl;

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->topicsAdvertised.erase(_topic);

  // Notify the discovery service to unregister and unadvertise my topic.
  this->dataPtr->shared->discovery->Unadvertise(
    _topic, this->dataPtr->nUuidStr);
}

//////////////////////////////////////////////////
int Node::Publish(const std::string &_topic, const ProtoMsg &_msg)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Topic not advertised before.
  if (this->dataPtr->topicsAdvertised.find(_topic) ==
      this->dataPtr->topicsAdvertised.end())
  {
    return -1;
  }

  // Local subscribers.
  std::map<std::string, ISubscriptionHandler_M> handlers;
  if (this->dataPtr->shared->localSubscriptions.GetHandlers(_topic, handlers))
  {
    for (auto &node : handlers)
    {
      for (auto &handler : node.second)
      {
        ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;

        if (subscriptionHandlerPtr)
          subscriptionHandlerPtr->RunLocalCallback(_topic, _msg);
        else
        {
          std::cerr << "Node::Publish(): Subscription handler is NULL"
                    << std::endl;
        }
      }
    }
  }

  // Remote subscribers.
  if (this->dataPtr->shared->remoteSubscribers.HasTopic(_topic))
  {
    std::string data;
    _msg.SerializeToString(&data);
    this->dataPtr->shared->Publish(_topic, data);
  }
  // Debug output.
  // else
  //  std::cout << "There are no remote subscribers...SKIP" << std::endl;

  return 0;
}

//////////////////////////////////////////////////
void Node::Unsubscribe(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  if (this->dataPtr->verbose)
    std::cout << "\nNode::Unsubscribe from [" << _topic << "]\n";

  this->dataPtr->shared->localSubscriptions.RemoveHandlersForNode(
    _topic, this->dataPtr->nUuidStr);

  // Remove the topic from the list of subscribed topics in this node.
  this->dataPtr->topicsSubscribed.resize(
    std::remove(this->dataPtr->topicsSubscribed.begin(),
      this->dataPtr->topicsSubscribed.end(), _topic) -
        this->dataPtr->topicsSubscribed.begin());

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->shared->localSubscriptions.HasHandlersForTopic(_topic))
  {
    this->dataPtr->shared->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, _topic.data(), _topic.size());
  }

  // Notify the publishers that I am no longer insterested in the topic.
  Addresses_M addresses;
  if (!this->dataPtr->shared->discovery->GetTopicAddresses(_topic, addresses))
    return;

  for (auto &proc : addresses)
  {
    for (auto &node : proc.second)
    {
      zmq::socket_t socket(*this->dataPtr->shared->context, ZMQ_DEALER);

      // Set ZMQ_LINGER to 0 means no linger period. Pending messages will be
      // discarded immediately when the socket is closed. That avoids infinite
      // waits if the publisher is disconnected.
      int lingerVal = 200;
      socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

      socket.connect(node.ctrl.c_str());

      zmq::message_t message;
      message.rebuild(_topic.size() + 1);
      memcpy(message.data(), _topic.c_str(), _topic.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      message.rebuild(this->dataPtr->shared->myAddress.size() + 1);
      memcpy(message.data(), this->dataPtr->shared->myAddress.c_str(),
             this->dataPtr->shared->myAddress.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      message.rebuild(this->dataPtr->nUuidStr.size() + 1);
      memcpy(message.data(), this->dataPtr->nUuidStr.c_str(),
        this->dataPtr->nUuidStr.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      std::string data = std::to_string(EndConnection);
      message.rebuild(data.size() + 1);
      memcpy(message.data(), data.c_str(), data.size() + 1);
      socket.send(message, 0);
    }
  }
}

//////////////////////////////////////////////////
void Node::UnadvertiseSrv(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->srvsAdvertised.resize(
    std::remove(this->dataPtr->srvsAdvertised.begin(),
      this->dataPtr->srvsAdvertised.end(), _topic) -
        this->dataPtr->srvsAdvertised.begin());

  // Remove all the REP handlers for this node.
  this->dataPtr->shared->repliers.RemoveHandlersForNode(
    _topic, this->dataPtr->nUuidStr);

  // Notify the discovery service to unregister and unadvertise my service call.
  this->dataPtr->shared->discovery->Unadvertise(
    _topic, this->dataPtr->nUuidStr);
}

//////////////////////////////////////////////////
bool Node::Interrupted()
{
  return this->dataPtr->shared->exit;
}
