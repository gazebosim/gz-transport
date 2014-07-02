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
#include <uuid/uuid.h>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Node::Node(const std::string &_ns)
  : dataPtr(new NodePrivate())
{
  if (TopicUtils::IsValidNamespace(_ns))
    this->dataPtr->ns = _ns;
  else
  {
    std::cerr << "Namespace [" << _ns << "] is not valid." << std::endl;
    std::cerr << "Using default namespace." << std::endl;
  }

  // Generate the node UUID.
  Uuid uuid;
  this->dataPtr->nUuid = uuid.ToString();
}

//////////////////////////////////////////////////
Node::~Node()
{
  // Unsubscribe from all the topics.
  while (!this->dataPtr->topicsSubscribed.empty())
  {
    auto topic = *this->dataPtr->topicsSubscribed.begin();
    this->Unsubscribe(topic);
  }

  // Unadvertise all my topics.
  while (!this->dataPtr->topicsAdvertised.empty())
  {
    auto topic = *this->dataPtr->topicsAdvertised.begin();
    this->Unadvertise(topic);
  }

  // Unadvertise all my services.
  while (!this->dataPtr->srvsAdvertised.empty())
  {
    auto topic = *this->dataPtr->srvsAdvertised.begin();
    this->UnadvertiseSrv(topic);
  }
}

//////////////////////////////////////////////////
/*bool Node::Advertise(const std::string &_topic, const Scope &_scope)
{
  std::string scTopic;
  if (!TopicUtils::GetScopedName(this->dataPtr->ns, _topic, scTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Add the topic to the list of advertised topics (if it was not before)
  this->dataPtr->topicsAdvertised.insert(scTopic);

  // Notify the discovery service to register and advertise my topic.
  this->dataPtr->shared->discovery->AdvertiseMsg(scTopic,
    this->dataPtr->shared->myAddress, this->dataPtr->shared->myControlAddress,
    this->dataPtr->nUuid, _scope);

  return true;
}*/

//////////////////////////////////////////////////
std::vector<std::string> Node::GetAdvertisedTopics()
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> v;

  for (auto i : this->dataPtr->topicsAdvertised)
    v.push_back(i);

  return v;
}

//////////////////////////////////////////////////
bool Node::Unadvertise(const std::string &_topic)
{
  std::string scTopic;
  if (!TopicUtils::GetScopedName(this->dataPtr->ns, _topic, scTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->topicsAdvertised.erase(scTopic);

  // Notify the discovery service to unregister and unadvertise my topic.
  this->dataPtr->shared->discovery->UnadvertiseMsg(
    scTopic, this->dataPtr->nUuid);

  return true;
}

//////////////////////////////////////////////////
bool Node::Publish(const std::string &_topic, const ProtoMsg &_msg)
{
  std::string scTopic;
  if (!TopicUtils::GetScopedName(this->dataPtr->ns, _topic, scTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Topic not advertised before.
  if (this->dataPtr->topicsAdvertised.find(scTopic) ==
      this->dataPtr->topicsAdvertised.end())
  {
    return false;
  }

  // Check types.
  std::shared_ptr<IAdvertiseHandler> advHandlerPtr;
  if (!this->dataPtr->advertisedHandlers.GetHandler(scTopic, advHandlerPtr))
    return false;

  if (!advHandlerPtr->CheckMsg(_msg))
    return false;

  // Local subscribers.
  std::map<std::string, ISubscriptionHandler_M> handlers;
  if (this->dataPtr->shared->localSubscriptions.GetHandlers(scTopic, handlers))
  {
    for (auto &node : handlers)
    {
      for (auto &handler : node.second)
      {
        ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;

        if (subscriptionHandlerPtr)
          subscriptionHandlerPtr->RunLocalCallback(scTopic, _msg);
        else
        {
          std::cerr << "Node::Publish(): Subscription handler is NULL"
                    << std::endl;
        }
      }
    }
  }

  // Remote subscribers.
  if (this->dataPtr->shared->remoteSubscribers.HasTopic(scTopic))
  {
    std::string data;
    _msg.SerializeToString(&data);
    this->dataPtr->shared->Publish(scTopic, data);
  }
  // Debug output.
  // else
  //  std::cout << "There are no remote subscribers...SKIP" << std::endl;

  return true;
}

//////////////////////////////////////////////////
std::map<std::string, Addresses_M> Node::GetSubscribedTopics()
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::map<std::string, Addresses_M> m;

  // I'm a real subscriber if I have interest in a topic and I know a publisher.
  for (auto topic : this->dataPtr->topicsSubscribed)
  {
    Addresses_M addresses;
    if (this->dataPtr->shared->discovery->GetMsgAddresses(topic, addresses))
      m[topic] = addresses;
  }

  return m;
}

//////////////////////////////////////////////////
bool Node::Unsubscribe(const std::string &_topic)
{
  std::string scTopic;
  if (!TopicUtils::GetScopedName(this->dataPtr->ns, _topic, scTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  this->dataPtr->shared->localSubscriptions.RemoveHandlersForNode(
    scTopic, this->dataPtr->nUuid);

  // Remove the topic from the list of subscribed topics in this node.
  this->dataPtr->topicsSubscribed.erase(scTopic);

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->shared->localSubscriptions.HasHandlersForTopic(scTopic))
  {
    this->dataPtr->shared->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, scTopic.data(), scTopic.size());
  }

  // Notify the publishers that I am no longer insterested in the topic.
  Addresses_M addresses;
  if (!this->dataPtr->shared->discovery->GetMsgAddresses(scTopic, addresses))
    return false;

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
      message.rebuild(scTopic.size() + 1);
      memcpy(message.data(), scTopic.c_str(), scTopic.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      message.rebuild(this->dataPtr->shared->myAddress.size() + 1);
      memcpy(message.data(), this->dataPtr->shared->myAddress.c_str(),
             this->dataPtr->shared->myAddress.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      message.rebuild(this->dataPtr->nUuid.size() + 1);
      memcpy(message.data(), this->dataPtr->nUuid.c_str(),
        this->dataPtr->nUuid.size() + 1);
      socket.send(message, ZMQ_SNDMORE);

      std::string data = std::to_string(EndConnection);
      message.rebuild(data.size() + 1);
      memcpy(message.data(), data.c_str(), data.size() + 1);
      socket.send(message, 0);
    }
  }

  return true;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::GetAdvertisedServices()
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> v;

  for (auto i : this->dataPtr->srvsAdvertised)
    v.push_back(i);

  return v;
}

//////////////////////////////////////////////////
bool Node::UnadvertiseSrv(const std::string &_topic)
{
  std::string scTopic;
  if (!TopicUtils::GetScopedName(this->dataPtr->ns, _topic, scTopic))
  {
    std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->srvsAdvertised.erase(scTopic);

  // Remove all the REP handlers for this node.
  this->dataPtr->shared->repliers.RemoveHandlersForNode(
    scTopic, this->dataPtr->nUuid);

  // Notify the discovery service to unregister and unadvertise my services.
  this->dataPtr->shared->discovery->UnadvertiseMsg(
    scTopic, this->dataPtr->nUuid);

  return true;
}
