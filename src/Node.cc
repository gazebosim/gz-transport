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

#include <algorithm>
#include <cassert>
#include <csignal>
#include <condition_variable>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "ignition/transport/MessageInfo.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

#include "NodeSharedPrivate.hh"

#ifdef _MSC_VER
#pragma warning(disable: 4503)
#endif

using namespace ignition;
using namespace transport;

namespace ignition
{
  namespace transport
  {
    /// \brief Flag to detect SIGINT or SIGTERM while the code is executing
    /// waitForShutdown().
    static bool g_shutdown = false;

    /// \brief Mutex to protect the boolean shutdown variable.
    static std::mutex g_shutdown_mutex;

    /// \brief Condition variable to wakeup waitForShutdown() and exit.
    static std::condition_variable g_shutdown_cv;

    //////////////////////////////////////////////////
    /// \brief Function executed when a SIGINT or SIGTERM signals are captured.
    /// \param[in] _signal Signal received.
    static void signal_handler(const int _signal)
    {
      if (_signal == SIGINT || _signal == SIGTERM)
      {
        g_shutdown_mutex.lock();
        g_shutdown = true;
        g_shutdown_mutex.unlock();
        g_shutdown_cv.notify_all();
      }
    }

    //////////////////////////////////////////////////
    /// \internal
    /// \brief Private data for Node::Publisher class.
    class Node::PublisherPrivate
    {
      /// \brief Default constructor.
      public: PublisherPrivate()
        : shared(NodeShared::Instance())
      {
      }

      /// \brief Constructor
      /// \param[in] _publisher The message publisher.
      public: explicit PublisherPrivate(const MessagePublisher &_publisher)
        : shared(NodeShared::Instance()),
          publisher(_publisher)
      {
      }

      /// \brief Check if this Publisher is ready to send an update based on
      /// publication settings and the clock.
      /// \return True if it is okay to publish, false otherwise.
      public: bool UpdateThrottling()
      {
        std::lock_guard<std::mutex> lk(this->mutex);
        if (!this->publisher.Options().Throttled())
          return true;

        Timestamp now = std::chrono::steady_clock::now();

        // Elapsed time since the last callback execution.
        auto elapsed = now - this->lastCbTimestamp;
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(
              elapsed).count() < this->periodNs)
        {
          return false;
        }

        // Update the last callback execution.
        this->lastCbTimestamp = now;
        return true;
      }

      /// \brief Check if this Publisher is valid
      /// \return True if we have a topic to publish to, otherwise false.
      public: bool Valid()
      {
        return !this->publisher.Topic().empty();
      }

      /// \brief Destructor.
      public: virtual ~PublisherPrivate()
      {
        std::lock_guard<std::recursive_mutex> lk(this->shared->mutex);
        // Notify the discovery service to unregister and unadvertise my topic.
        if (!this->shared->dataPtr->msgDiscovery->Unadvertise(
               this->publisher.Topic(), this->publisher.NUuid()))
        {
          std::cerr << "~PublisherPrivate() Error unadvertising topic ["
                    << this->publisher.Topic() << "]" << std::endl;
        }
      }

      /// \brief Create a MessageInfo object for this Publisher
      MessageInfo CreateMessageInfo()
      {
        MessageInfo info;

        // Set the topic and the partition at the same time
        info.SetTopicAndPartition(this->publisher.Topic());

        // Set the message type name
        info.SetType(this->publisher.MsgTypeName());

        return info;
      }

      /// \brief Pointer to the object shared between all the nodes within the
      /// same process.
      public: NodeShared *shared = nullptr;

      /// \brief The message publisher.
      public: MessagePublisher publisher;

      /// \brief Timestamp of the last callback executed.
      public: Timestamp lastCbTimestamp;

      /// \brief If throttling is enabled, the minimum period for receiving a
      /// message in nanoseconds.
      public: double periodNs = 0.0;

      /// \brief Mutex to protect the node::publisher from race conditions.
      public: std::mutex mutex;
    };
  }
}

//////////////////////////////////////////////////
void ignition::transport::waitForShutdown()
{
  // Install a signal handler for SIGINT and SIGTERM.
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::unique_lock<std::mutex> lk(g_shutdown_mutex);
  g_shutdown_cv.wait(lk, []{return g_shutdown;});
}

//////////////////////////////////////////////////
Node::Publisher::Publisher()
  : dataPtr(std::make_shared<PublisherPrivate>())
{
}

//////////////////////////////////////////////////
Node::Publisher::Publisher(const MessagePublisher &_publisher)
  : dataPtr(std::make_shared<PublisherPrivate>(_publisher))
{
  if (this->dataPtr->publisher.Options().Throttled())
  {
    this->dataPtr->periodNs =
      1e9 / this->dataPtr->publisher.Options().MsgsPerSec();
  }
}

//////////////////////////////////////////////////
Node::Publisher::~Publisher()
{
}

//////////////////////////////////////////////////
Node::Publisher::operator bool()
{
  return this->Valid();
}

//////////////////////////////////////////////////
Node::Publisher::operator bool() const
{
  return this->Valid();
}

//////////////////////////////////////////////////
bool Node::Publisher::Valid() const
{
  return this->dataPtr->Valid();
}

//////////////////////////////////////////////////
bool Node::Publisher::HasConnections() const
{
  auto &publisher = this->dataPtr->publisher;
  const std::string &topic = publisher.Topic();
  const std::string &msgType = publisher.MsgTypeName();

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  /// \todo: Checking "remoteSubscribers.HasTopic()" will return true even
  /// if the subscriber has not successfully authenticated with the
  /// publisher.
  /// See Issue #73
  return this->Valid() &&
    (this->dataPtr->shared->localSubscribers.HasSubscriber(topic, msgType) ||
     this->dataPtr->shared->remoteSubscribers.HasTopic(topic, msgType));
}

//////////////////////////////////////////////////
bool Node::Publisher::Publish(const ProtoMsg &_msg)
{
  if (!this->Valid())
    return false;

  const std::string &publisherMsgType = this->dataPtr->publisher.MsgTypeName();

  // Check that the msg type matches the topic type previously advertised.
  if (publisherMsgType != _msg.GetTypeName())
  {
    std::cerr << "Node::Publisher::Publish() Type mismatch.\n"
              << "\t* Type advertised: "
              << this->dataPtr->publisher.MsgTypeName()
              << "\n\t* Type published: " << _msg.GetTypeName() << std::endl;
    return false;
  }

  // Check the publication throttling option.
  if (!this->UpdateThrottling())
    return true;

  const std::string &publisherTopic = this->dataPtr->publisher.Topic();

  const NodeShared::SubscriberInfo &subscribers =
      this->dataPtr->shared->CheckSubscriberInfo(
        publisherTopic, publisherMsgType);

  // The serialized message size and buffer.
  const std::size_t msgSize = static_cast<std::size_t>(_msg.ByteSize());
  char *msgBuffer = nullptr;

  // Only serialize the message if we have a raw subscriber or a remote
  // subscriber.
  if (subscribers.haveRaw || subscribers.haveRemote)
  {
    // Allocate the buffer to store the serialized data.
    msgBuffer = static_cast<char *>(new char[msgSize]);

    // Fail out early if we are unable to serialize the message. We do not
    // want to send a corrupt/bad message to some subscribers and not others.
    if (!_msg.SerializeToArray(msgBuffer, msgSize))
    {
      delete[] msgBuffer;
      std::cerr << "Node::Publisher::Publish(): Error serializing data"
                << std::endl;
      return false;
    }
  }

  // Local and raw subscribers.
  if (subscribers.haveLocal || subscribers.haveRaw)
  {
    // Create and populate the message information object.
    // This must be a shared pointer so that we can pass it to
    // multiple threads below, and then allow this function to go
    // out of scope.
    std::shared_ptr<MessageInfo> info = std::make_shared<MessageInfo>(
        this->dataPtr->CreateMessageInfo());

    std::shared_ptr<ProtoMsg> msgCopy(_msg.New());
    msgCopy->CopyFrom(_msg);

    if (subscribers.haveLocal)
    {
      for (auto &node : subscribers.localHandlers)
      {
        for (auto &handler : node.second)
        {
          const ISubscriptionHandlerPtr &localHandler = handler.second;

          if (!localHandler)
          {
            std::cerr << "Node::Publisher::Publish(): "
                      << "NULL local subscription handler" << std::endl;
            continue;
          }

          if (localHandler->TypeName() != kGenericMessageType &&
              localHandler->TypeName() != _msg.GetTypeName())
          {
            continue;
          }

          // Launch local callback in a thread. We get the raw pointer to
          // the subscription handler because the object itself will change
          // in this loop.
          //
          // This supports asynchronous intraprocess callbacks,
          // which has the same behavior as interprocess callbacks.
          this->dataPtr->shared->dataPtr->workerPool.AddWork(
              [localHandler, msgCopy, info] ()
              {
                try
                {
                  localHandler->RunLocalCallback(*msgCopy, *info);
                }
                catch (...)
                {
                  std::cerr << "Exception occurred in a local callback "
                    << "on topic [" << info->Topic() << "] with message ["
                    << msgCopy->DebugString() << "]" << std::endl;
                }
              });
        }
      }
    }

    if (subscribers.haveRaw)
    {
      std::shared_ptr<char> sharedBuffer;
      for (auto &node : subscribers.rawHandlers)
      {
        for (auto &handler : node.second)
        {
          const RawSubscriptionHandlerPtr &rawHandler = handler.second;

          if (!rawHandler)
          {
            std::cerr << "Node::Publisher::Publish(): "
                      << "NULL raw subscription handler" << std::endl;
            continue;
          }

          if (rawHandler->TypeName() != kGenericMessageType &&
              rawHandler->TypeName() != _msg.GetTypeName())
          {
            continue;
          }

          if (!sharedBuffer)
          {
            // If the sharedBuffer has not been created, do so now.
            sharedBuffer = std::shared_ptr<char>(
                  new char[msgSize], std::default_delete<char[]>());
            memcpy(sharedBuffer.get(), msgBuffer, msgSize);
          }

          this->dataPtr->shared->dataPtr->workerPool.AddWork(
              [rawHandler, sharedBuffer, msgSize, info, msgCopy] ()
              {
                try
                {
                  rawHandler->RunRawCallback(
                        sharedBuffer.get(), msgSize, *info);
                }
                catch (...)
                {
                  std::cerr << "Exception occured in a local raw callback "
                            << "on topic [" << info->Topic() << "] with "
                            << "message [" << msgCopy->DebugString() << "]"
                            << std::endl;
                }
              });
        }
      }
    }
  }

  // Handle remote subscribers.
  if (subscribers.haveRemote)
  {
    // Zmq will call this lambda when the message is published.
    // We use it to deallocate the buffer.
    auto myDeallocator = [](void *_buffer, void *)
    {
      delete[] reinterpret_cast<char*>(_buffer);
    };

    if (!this->dataPtr->shared->Publish(this->dataPtr->publisher.Topic(),
          msgBuffer, msgSize, myDeallocator, _msg.GetTypeName()))
    {
      return false;
    }
  }
  else
  {
    delete[] msgBuffer;
  }

  return true;
}

//////////////////////////////////////////////////
bool Node::Publisher::PublishRaw(
    const std::string &_msgData,
    const std::string &_msgType)
{
  if (!this->dataPtr->Valid())
    return false;

  const std::string &publisherMsgType = this->dataPtr->publisher.MsgTypeName();

  if (publisherMsgType  != _msgType && publisherMsgType != kGenericMessageType)
  {
    std::cerr << "Node::Publisher::PublishRaw() type mismatch.\n"
              << "\t* Type advertised: "
              << this->dataPtr->publisher.MsgTypeName()
              << "\n\t* Type published: " << _msgType << std::endl;
    return false;
  }

  if (!this->dataPtr->UpdateThrottling())
    return true;

  const std::string &topic = this->dataPtr->publisher.Topic();

  const NodeShared::SubscriberInfo &subscribers =
      this->dataPtr->shared->CheckSubscriberInfo(topic, _msgType);

  // Trigger local subscribers.
  this->dataPtr->shared->TriggerSubscriberCallbacks(
        topic, _msgData, _msgType, subscribers);

  // Remote subscribers. Note that the data is already presumed to be
  // serialized, so we just pass it along for publication.
  if (subscribers.haveRemote)
  {
    const std::size_t msgSize = _msgData.size();
    char *msgBuffer = static_cast<char *>(new char[msgSize]);
    memcpy(msgBuffer, _msgData.c_str(), msgSize);
    auto myDeallocator = [](void *_buffer, void * /*_hint*/)
    {
      delete[] reinterpret_cast<char*>(_buffer);
    };

    // Note: This will copy _msgData (i.e. not zero copy)
    if (!this->dataPtr->shared->Publish(
          this->dataPtr->publisher.Topic(),
          msgBuffer, msgSize, myDeallocator, _msgType))
    {
      return false;
    }
  }

  return true;
}

//////////////////////////////////////////////////
bool Node::Publisher::UpdateThrottling()
{
  return this->dataPtr->UpdateThrottling();
}

//////////////////////////////////////////////////
Node::Node(const NodeOptions &_options)
  : dataPtr(new NodePrivate())
{
  // Generate the node UUID.
  Uuid uuid;
  this->dataPtr->nUuid = uuid.ToString();

  // Save the options.
  this->dataPtr->options = _options;
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

  // The list of advertised topics should be empty.
  assert(this->AdvertisedTopics().empty());

  // Unadvertise all my services.
  auto advServices = this->AdvertisedServices();
  for (auto const &service : advServices)
  {
    if (!this->UnadvertiseSrv(service))
    {
      std::cerr << "Node::~Node(): Error unadvertising service ["
                << service << "]" << std::endl;
    }
  }

  // The list of advertised services should be empty.
  assert(this->AdvertisedServices().empty());
}

//////////////////////////////////////////////////
std::vector<std::string> Node::AdvertisedTopics() const
{
  std::vector<std::string> v;
  std::unordered_set<std::string> result;
  std::vector<MessagePublisher> pubs;

  {
    std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

    auto pUUID = this->dataPtr->shared->pUuid;
    auto &info = this->dataPtr->shared->dataPtr->msgDiscovery->Info();
    info.PublishersByNode(pUUID, this->NodeUuid(), pubs);
  }

  // Copy the topics to a std::set for removing duplications.
  for (auto const &pub : pubs)
    result.insert(pub.Topic());

  // Remove the partition information and convert to std::vector.
  for (auto topic : result)
  {
    topic.erase(0, topic.find_last_of("@") + 1);
    v.push_back(topic);
  }

  return v;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::SubscribedTopics() const
{
  std::vector<std::string> v;

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

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
  if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the subscribers for the given topic that belong to this node.
  this->dataPtr->shared->localSubscribers.RemoveHandlersForNode(
        fullyQualifiedTopic, this->dataPtr->nUuid);

  // Remove the topic from the list of subscribed topics in this node.
  this->dataPtr->topicsSubscribed.erase(fullyQualifiedTopic);

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->shared->localSubscribers
      .HasSubscriber(fullyQualifiedTopic))
  {
    this->dataPtr->shared->dataPtr->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, fullyQualifiedTopic.data(), fullyQualifiedTopic.size());
  }

  // Notify to the publishers that I am no longer interested in the topic.
  MsgAddresses_M addresses;
  if (!this->dataPtr->shared->dataPtr->msgDiscovery->Publishers(
        fullyQualifiedTopic, addresses))
  {
    return false;
  }

  for (auto &proc : addresses)
  {
    for (auto &node : proc.second)
    {
      zmq::socket_t socket(*this->dataPtr->shared->dataPtr->context,
          ZMQ_DEALER);

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

      msg.rebuild(kGenericMessageType.size());
      memcpy(msg.data(), kGenericMessageType.data(),
             kGenericMessageType.size());
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
  std::vector<std::string> v;

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

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
  if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->srvsAdvertised.erase(fullyQualifiedTopic);

  // Remove all the REP handlers for this node.
  this->dataPtr->shared->repliers.RemoveHandlersForNode(
    fullyQualifiedTopic, this->dataPtr->nUuid);

  // Notify the discovery service to unregister and unadvertise my services.
  if (!this->dataPtr->shared->dataPtr->srvDiscovery->Unadvertise(
        fullyQualifiedTopic, this->dataPtr->nUuid))
  {
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
void Node::TopicList(std::vector<std::string> &_topics) const
{
  std::vector<std::string> allTopics;
  _topics.clear();

  this->dataPtr->shared->dataPtr->msgDiscovery->TopicList(allTopics);

  for (auto &topic : allTopics)
  {
    // Get the partition name.
    std::string partition = topic.substr(1, topic.find_last_of("@") - 1);
    // Remove the front '/'
    if (!partition.empty())
      partition.erase(partition.begin());

    // Discard if the partition name does not match this node's partition.
    if (partition != this->Options().Partition())
      continue;

    // Remove the partition part from the topic.
    topic.erase(0, topic.find_last_of("@") + 1);

    _topics.push_back(topic);
  }
}

//////////////////////////////////////////////////
void Node::ServiceList(std::vector<std::string> &_services) const
{
  std::vector<std::string> allServices;
  _services.clear();

  this->dataPtr->shared->dataPtr->srvDiscovery->TopicList(allServices);

  for (auto &service : allServices)
  {
    // Get the partition name.
    std::string partition = service.substr(1, service.find_last_of("@") - 1);
    // Remove the front '/'
    if (!partition.empty())
      partition.erase(partition.begin());

    // Discard if the partition name does not match this node's partition.
    if (partition != this->Options().Partition())
      continue;

    // Remove the partition part from the service.
    service.erase(0, service.find_last_of("@") + 1);

    _services.push_back(service);
  }
}

//////////////////////////////////////////////////
bool Node::SubscribeRaw(
    const std::string &_topic,
    const RawCallback &_callback,
    const std::string &_msgType,
    const SubscribeOptions &_opts)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::FullyQualifiedName(this->dataPtr->options.Partition(),
                                      this->dataPtr->options.NameSpace(),
                                      _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  const std::shared_ptr<RawSubscriptionHandler> handlerPtr =
      std::make_shared<RawSubscriptionHandler>(
        this->dataPtr->nUuid, _msgType, _opts);

  handlerPtr->SetCallback(_callback);

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  this->dataPtr->shared->localSubscribers.raw.AddHandler(
        fullyQualifiedTopic, this->dataPtr->nUuid, handlerPtr);

  return this->dataPtr->SubscribeHelper(fullyQualifiedTopic);
}

//////////////////////////////////////////////////
// FIXME: Node::Partition() and Node::NameSpace() appear to be undefined

//////////////////////////////////////////////////
NodeShared *Node::Shared() const
{
  return this->dataPtr->shared;
}

//////////////////////////////////////////////////
const std::string &Node::NodeUuid() const
{
  return this->dataPtr->nUuid;
}

//////////////////////////////////////////////////
std::unordered_set<std::string> &Node::TopicsSubscribed() const
{
  return this->dataPtr->topicsSubscribed;
}

//////////////////////////////////////////////////
std::unordered_set<std::string> &Node::SrvsAdvertised() const
{
  return this->dataPtr->srvsAdvertised;
}

//////////////////////////////////////////////////
NodeOptions &Node::Options() const
{
  return this->dataPtr->options;
}

//////////////////////////////////////////////////
bool Node::TopicInfo(const std::string &_topic,
                     std::vector<MessagePublisher> &_publishers) const
{
  this->dataPtr->shared->dataPtr->msgDiscovery->WaitForInit();

  // Construct a topic name with the partition and namespace
  std::string fullyQualifiedTopic;
  if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Get all the publishers on the given topics
  MsgAddresses_M pubs;
  if (!this->dataPtr->shared->dataPtr->msgDiscovery->Publishers(
        fullyQualifiedTopic, pubs))
  {
    return false;
  }

  _publishers.clear();

  // Copy the publishers.
  for (MsgAddresses_M::iterator iter = pubs.begin(); iter != pubs.end(); ++iter)
  {
    for (std::vector<MessagePublisher>::iterator pubIter = iter->second.begin();
         pubIter != iter->second.end(); ++pubIter)
    {
      // Add the publisher if it doesn't already exist.
      if (std::find(_publishers.begin(), _publishers.end(), *pubIter) ==
          _publishers.end())
      {
        _publishers.push_back(*pubIter);
      }
    }
  }

  return true;
}

//////////////////////////////////////////////////
bool Node::ServiceInfo(const std::string &_service,
                       std::vector<ServicePublisher> &_publishers) const
{
  this->dataPtr->shared->dataPtr->srvDiscovery->WaitForInit();

  // Construct a topic name with the partition and namespace
  std::string fullyQualifiedTopic;
  if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _service, fullyQualifiedTopic))
  {
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Get all the publishers on the given service.
  SrvAddresses_M pubs;
  if (!this->dataPtr->shared->dataPtr->srvDiscovery->Publishers(
        fullyQualifiedTopic, pubs))
  {
    return false;
  }

  _publishers.clear();

  // Copy the publishers.
  for (SrvAddresses_M::iterator iter = pubs.begin(); iter != pubs.end(); ++iter)
  {
    for (std::vector<ServicePublisher>::iterator pubIter = iter->second.begin();
         pubIter != iter->second.end(); ++pubIter)
    {
      // Add the publisher if it doesn't already exist.
      if (std::find(_publishers.begin(), _publishers.end(), *pubIter) ==
          _publishers.end())
      {
        _publishers.push_back(*pubIter);
      }
    }
  }

  return true;
}

/////////////////////////////////////////////////
Node::Publisher Node::Advertise(const std::string &_topic,
    const std::string &_msgTypeName, const AdvertiseMessageOptions &_options)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::FullyQualifiedName(this->Options().Partition(),
        this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return Publisher();
  }

  auto currentTopics = this->AdvertisedTopics();

  if (std::find(currentTopics.begin(), currentTopics.end(),
        fullyQualifiedTopic) != currentTopics.end())
  {
    std::cerr << "Topic [" << _topic << "] already advertised. You cannot"
      << " advertise the same topic twice on the same node."
      << " If you want to advertise the same topic with different"
      << " types, use separate nodes" << std::endl;
    return Publisher();
  }

  std::lock_guard<std::recursive_mutex> lk(this->Shared()->mutex);

  // Notify the discovery service to register and advertise my topic.
  MessagePublisher publisher(fullyQualifiedTopic,
      this->Shared()->myAddress,
      this->Shared()->myControlAddress,
      this->Shared()->pUuid, this->NodeUuid(), _msgTypeName, _options);

  if (!this->Shared()->dataPtr->msgDiscovery->Advertise(publisher))
  {
    std::cerr << "Node::Advertise(): Error advertising topic ["
      << _topic
      << "]. Did you forget to start the discovery service?"
      << std::endl;
    return Publisher();
  }

  return Publisher(publisher);
}

//////////////////////////////////////////////////
bool NodePrivate::SubscribeHelper(const std::string &_fullyQualifiedTopic)
{
  // Add the topic to the list of subscribed topics (if it was not before).
  this->topicsSubscribed.insert(_fullyQualifiedTopic);

  // Discover the list of nodes that publish on the topic.
  if (!this->shared->dataPtr->msgDiscovery->Discover(_fullyQualifiedTopic))
  {
    std::cerr << "Node::Subscribe(): Error discovering topic ["
              << _fullyQualifiedTopic
              << "]. Did you forget to start the discovery service?"
              << std::endl;
    return false;
  }

  return true;
}

/////////////////////////////////////////////////
bool Node::SubscribeHelper(const std::string &_fullyQualifiedTopic)
{
  return this->dataPtr->SubscribeHelper(_fullyQualifiedTopic);
}
