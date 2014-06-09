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
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Node::Node(bool _verbose)
  : dataPtr(NodePrivate::GetInstance(_verbose))
{
  uuid_generate(this->nUuid);
  this->nUuidStr = GetGuidStr(this->nUuid);
}

//////////////////////////////////////////////////
Node::~Node()
{
  // Unsubscribe from all the topics.
  for (auto topic : this->topicsSubscribed)
    this->Unsubscribe(topic);

  // Unadvertise all my topics.
  for (auto topic : this->topicsAdvertised)
    this->Unadvertise(topic);
}

//////////////////////////////////////////////////
void Node::Advertise(const std::string &_topic, const Scope &_scope)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // Add the topic to the list of advertised topics (if it was not before)
  if (std::find(this->topicsAdvertised.begin(),
    this->topicsAdvertised.end(), _topic) == this->topicsAdvertised.end())
  {
    this->topicsAdvertised.push_back(_topic);
  }

  // Notify the discovery service to register and advertise my topic.
  this->dataPtr->discovery->Advertise(AdvertiseType::Msg, _topic,
    this->dataPtr->myAddress, this->dataPtr->myControlAddress, this->nUuidStr,
    _scope);
}

//////////////////////////////////////////////////
void Node::Unadvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->topicsAdvertised.resize(
    std::remove(this->topicsAdvertised.begin(), this->topicsAdvertised.end(),
      _topic) - this->topicsAdvertised.begin());

  // Notify the discovery service to unregister and unadvertise my topic.
  this->dataPtr->discovery->Unadvertise(_topic, this->nUuidStr);
}

//////////////////////////////////////////////////
int Node::Publish(const std::string &_topic, const ProtoMsg &_msg)
{
  assert(_topic != "");

  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // Topic not advertised before.
  if (std::find(this->topicsAdvertised.begin(),
    this->topicsAdvertised.end(), _topic) == this->topicsAdvertised.end())
  {
    return -1;
  }

  // Local subscribers.
  ISubscriptionHandler_M handlers;
  this->dataPtr->localSubscriptions.GetSubscriptionHandlers(_topic, handlers);
  for (auto handler : handlers)
  {
    ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;

    if (subscriptionHandlerPtr)
      subscriptionHandlerPtr->RunLocalCallback(_topic, _msg);
    else
      std::cerr << "Node::Publish(): Subscription handler is NULL" << std::endl;
  }

  // Remote subscribers.
  if (this->dataPtr->remoteSubscribers.HasTopic(_topic))
  {
    std::string data;
    _msg.SerializeToString(&data);
    this->dataPtr->Publish(_topic, data);
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

  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  if (this->dataPtr->verbose)
    std::cout << "\nUnsubscribe (" << _topic << ")\n";

  this->dataPtr->localSubscriptions.RemoveSubscriptionHandler(
    _topic, this->nUuidStr);

  // Remove the topic from the list of subscribed topics in this node.
  this->topicsSubscribed.resize(
    std::remove(this->topicsSubscribed.begin(), this->topicsSubscribed.end(),
      _topic) - this->topicsSubscribed.begin());

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->localSubscriptions.Subscribed(_topic))
  {
    this->dataPtr->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, _topic.data(), _topic.size());
  }

  // Notify the publishers that I am no longer insterested in the topic.
  Addresses_M addresses;
  if (!this->dataPtr->discovery->GetTopicAddresses(_topic, addresses))
    return;

  for (auto &proc : addresses)
  {
    for (auto &node : proc.second)
    {
      zmq::socket_t socket(*this->dataPtr->context, ZMQ_DEALER);

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

      // Not needed.
      message.rebuild(this->dataPtr->myAddress.size() + 1);
      memcpy(message.data(), this->dataPtr->myAddress.c_str(),
             this->dataPtr->myAddress.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      message.rebuild(this->nUuidStr.size() + 1);
      memcpy(message.data(), this->nUuidStr.c_str(), this->nUuidStr.size() + 1);
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

  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->srvsAdvertised.resize(
    std::remove(this->srvsAdvertised.begin(), this->srvsAdvertised.end(),
      _topic) - this->srvsAdvertised.begin());

  // Remove all the REP handlers for this node.
  this->dataPtr->repliers.RemoveRepHandler(_topic, this->nUuidStr);

  // Notify the discovery service to unregister and unadvertise my service call.
  this->dataPtr->discovery->Unadvertise(_topic, this->nUuidStr);
}

//////////////////////////////////////////////////
bool Node::Interrupted()
{
  return this->dataPtr->exit;
}
