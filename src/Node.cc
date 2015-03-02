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
#include <google/protobuf/message.h>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Node::Node()
  : dataPtr(new NodePrivate())
{
  // Check if the environment variable IGN_PARTITION is present.
  std::string partitionStr;
  char *envPartition = std::getenv("IGN_PARTITION");

  if (envPartition)
  {
    partitionStr = std::string(envPartition);
    if (TopicUtils::IsValidNamespace(partitionStr))
      this->dataPtr->partition = partitionStr;
    else
      std::cerr << "Invalid IGN_PARTITION value [" << partitionStr << "]"
                << std::endl;
  }

  // Generate the node UUID.
  Uuid uuid;
  this->dataPtr->nUuid = uuid.ToString();
}

//////////////////////////////////////////////////
Node::Node(const std::string &_partition, const std::string &_ns)
  : Node()
{
  if (TopicUtils::IsValidNamespace(_ns))
    this->dataPtr->ns = _ns;
  else
  {
    std::cerr << "Namespace [" << _ns << "] is not valid." << std::endl;
    std::cerr << "Using default namespace." << std::endl;
  }

  if (TopicUtils::IsValidNamespace(_partition))
    this->dataPtr->partition = _partition;
  else
  {
    std::cerr << "Partition [" << _partition << "] is not valid." << std::endl;
    std::cerr << "Using default partition." << std::endl;
  }

  // Generate the node UUID.
  Uuid uuid;
  this->dataPtr->nUuid = uuid.ToString();
}

//////////////////////////////////////////////////
Node::~Node()
{
  // Unsubscribe from all the topics.
  auto subsTopics = this->SubscribedTopics();
  for (auto const &topic : subsTopics)
    this->Unsubscribe(topic);

  // The list of subscribed topics should be empty.
  assert(this->SubscribedTopics().empty());

  // Unadvertise all my topics.
  auto advTopics = this->AdvertisedTopics();
  for (auto const &topic : advTopics)
    this->Unadvertise(topic);

  // The list of advertised topics should be empty.
  assert(this->AdvertisedTopics().empty());

  // Unadvertise all my services.
  auto advServices = this->AdvertisedServices();
  for (auto const &service : advServices)
    this->UnadvertiseSrv(service);

  // The list of advertised services should be empty.
  assert(this->AdvertisedServices().empty());
}

//////////////////////////////////////////////////
bool Node::Advertise(const std::string &_topic, const Scope_t &_scope)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
    this->dataPtr->ns, _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> discLk(
          this->dataPtr->shared->discovery->Mutex());
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Add the topic to the list of advertised topics (if it was not before).
  this->dataPtr->topicsAdvertised.insert(fullyQualifiedTopic);

  // Notify the discovery service to register and advertise my topic.
  MessagePublisher publisher(fullyQualifiedTopic,
    this->dataPtr->shared->myAddress, this->dataPtr->shared->myControlAddress,
    this->dataPtr->shared->pUuid, this->dataPtr->nUuid, _scope, "unused");
  this->dataPtr->shared->discovery->AdvertiseMsg(publisher);

  return true;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::AdvertisedTopics() const
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> v;

  for (auto topic : this->dataPtr->topicsAdvertised)
  {
    // Remove the partition information.
    topic.erase(0, topic.find_last_of("@") + 1);
    v.push_back(topic);
  }

  return v;
}

//////////////////////////////////////////////////
bool Node::Unadvertise(const std::string &_topic)
{
  std::string fullyQualifiedTopic = _topic;
  if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
    this->dataPtr->ns, _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> discLk(
          this->dataPtr->shared->discovery->Mutex());
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->topicsAdvertised.erase(fullyQualifiedTopic);

  // Notify the discovery service to unregister and unadvertise my topic.
  this->dataPtr->shared->discovery->UnadvertiseMsg(fullyQualifiedTopic,
    this->dataPtr->nUuid);

  return true;
}

//////////////////////////////////////////////////
bool Node::Publish(const std::string &_topic, const ProtoMsg &_msg)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
    this->dataPtr->ns, _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Topic not advertised before.
  if (this->dataPtr->topicsAdvertised.find(fullyQualifiedTopic) ==
      this->dataPtr->topicsAdvertised.end())
  {
    return false;
  }

  // Local subscribers.
  std::map<std::string, ISubscriptionHandler_M> handlers;
  if (this->dataPtr->shared->localSubscriptions.GetHandlers(fullyQualifiedTopic,
        handlers))
  {
    for (auto &node : handlers)
    {
      for (auto &handler : node.second)
      {
        ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;

        if (subscriptionHandlerPtr)
          subscriptionHandlerPtr->RunLocalCallback(fullyQualifiedTopic, _msg);
        else
        {
          std::cerr << "Node::Publish(): Subscription handler is NULL"
                    << std::endl;
        }
      }
    }
  }

  // Remote subscribers.
  if (this->dataPtr->shared->remoteSubscribers.HasTopic(fullyQualifiedTopic))
  {
    std::string data;
    _msg.SerializeToString(&data);
    this->dataPtr->shared->Publish(fullyQualifiedTopic, data);
  }
  // Debug output.
  // else
  //   std::cout << "There are no remote subscribers...SKIP" << std::endl;

  return true;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::SubscribedTopics() const
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> v;

  // I'm a real subscriber if I have interest in a topic and I know a publisher.
  for (auto topic : this->dataPtr->topicsSubscribed)
  {
    // Remove the partition information from the topic.
    topic.erase(0, topic.find_last_of("@") + 1);
    v.push_back(topic);
  }

  return v;
}

//////////////////////////////////////////////////
bool Node::Unsubscribe(const std::string &_topic)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
    this->dataPtr->ns, _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  this->dataPtr->shared->localSubscriptions.RemoveHandlersForNode(
    fullyQualifiedTopic, this->dataPtr->nUuid);

  // Remove the topic from the list of subscribed topics in this node.
  this->dataPtr->topicsSubscribed.erase(fullyQualifiedTopic);

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->shared->localSubscriptions.HasHandlersForTopic(
    fullyQualifiedTopic))
  {
    this->dataPtr->shared->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, fullyQualifiedTopic.data(), fullyQualifiedTopic.size());
  }

  // Notify to the publishers that I am no longer interested in the topic.
  MsgAddresses_M addresses;
  if (!this->dataPtr->shared->discovery->MsgPublishers(fullyQualifiedTopic,
    addresses))
  {
    return false;
  }

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

      socket.connect(node.Ctrl().c_str());

      zmq::message_t msg;
      msg.rebuild(fullyQualifiedTopic.size());
      memcpy(msg.data(), fullyQualifiedTopic.data(),
        fullyQualifiedTopic.size());
      socket.send(msg, ZMQ_SNDMORE);

      msg.rebuild(this->dataPtr->shared->myAddress.size());
      memcpy(msg.data(), this->dataPtr->shared->myAddress.data(),
             this->dataPtr->shared->myAddress.size());
      socket.send(msg, ZMQ_SNDMORE);

      msg.rebuild(this->dataPtr->nUuid.size());
      memcpy(msg.data(), this->dataPtr->nUuid.data(),
             this->dataPtr->nUuid.size());
      socket.send(msg, ZMQ_SNDMORE);

      std::string data = std::to_string(EndConnection);
      msg.rebuild(data.size());
      memcpy(msg.data(), data.data(), data.size());
      socket.send(msg, 0);
    }
  }

  return true;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::AdvertisedServices() const
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> v;

  for (auto service : this->dataPtr->srvsAdvertised)
  {
    // Remove the partition information from the service name.
    service.erase(0, service.find_last_of("@") + 1);
    v.push_back(service);
  }

  return v;
}

//////////////////////////////////////////////////
bool Node::UnadvertiseSrv(const std::string &_topic)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->dataPtr->partition,
    this->dataPtr->ns, _topic, fullyQualifiedTopic))
  {
    std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> discLk(
          this->dataPtr->shared->discovery->Mutex());
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->srvsAdvertised.erase(fullyQualifiedTopic);

  // Remove all the REP handlers for this node.
  this->dataPtr->shared->repliers.RemoveHandlersForNode(
    fullyQualifiedTopic, this->dataPtr->nUuid);

  // Notify the discovery service to unregister and unadvertise my services.
  this->dataPtr->shared->discovery->UnadvertiseSrv(fullyQualifiedTopic,
    this->dataPtr->nUuid);

  return true;
}

//////////////////////////////////////////////////
void Node::TopicList(std::vector<std::string> &_topics) const
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> allTopics;
  this->dataPtr->shared->discovery->TopicList(allTopics);

  _topics.clear();
  for (auto &topic : allTopics)
  {
    // Get the partition name.
    std::string partition = topic.substr(1, topic.find_last_of("@") - 1);
    // Remove the front '/'
    if (!partition.empty())
      partition.erase(partition.begin());

    // Discard if the partition name does not match this node's partition.
    if (partition != this->Partition())
      continue;

    // Remove the partition part from the topic.
    topic.erase(0, topic.find_last_of("@") + 1);

    _topics.push_back(topic);
  }
}

//////////////////////////////////////////////////
void Node::ServiceList(std::vector<std::string> &_services) const
{
  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  std::vector<std::string> allServices;
  this->dataPtr->shared->discovery->ServiceList(allServices);

  _services.clear();
  for (auto &service : allServices)
  {
    // Get the partition name.
    std::string partition = service.substr(1, service.find_last_of("@") - 1);
    // Remove the front '/'
    if (!partition.empty())
      partition.erase(partition.begin());

    // Discard if the partition name does not match this node's partition.
    if (partition != this->Partition())
      continue;

    // Remove the partition part from the service.
    service.erase(0, service.find_last_of("@") + 1);

    _services.push_back(service);
  }
}

//////////////////////////////////////////////////
const std::string& Node::Partition() const
{
  return this->dataPtr->partition;
}

//////////////////////////////////////////////////
const std::string& Node::NameSpace() const
{
  return this->dataPtr->ns;
}

//////////////////////////////////////////////////
NodeShared* Node::Shared() const
{
  return this->dataPtr->shared;
}

//////////////////////////////////////////////////
const std::string& Node::NodeUuid() const
{
  return this->dataPtr->nUuid;
}

//////////////////////////////////////////////////
std::unordered_set<std::string>& Node::TopicsSubscribed() const
{
  return this->dataPtr->topicsSubscribed;
}

//////////////////////////////////////////////////
std::unordered_set<std::string>& Node::SrvsAdvertised() const
{
  return this->dataPtr->srvsAdvertised;
}
