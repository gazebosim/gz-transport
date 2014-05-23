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

//////////////////////////////////////////////////
transport::Node::Node(bool _verbose)
  : dataPtr(transport::NodePrivate::GetInstance(_verbose))
{
  uuid_generate(this->nodeUuid);
  this->nodeUuidStr = transport::GetGuidStr(this->nodeUuid);
}

//////////////////////////////////////////////////
transport::Node::~Node()
{
  for (auto topicInfo : this->dataPtr->topics.GetTopicsInfo())
  {
    zbeacon_t *topicBeacon = nullptr;
    if (this->dataPtr->topics.GetBeacon(topicInfo.first, &topicBeacon))
    {
      // Destroy the beacon.
      zbeacon_silence(topicBeacon);
      zbeacon_destroy(&topicBeacon);
      this->dataPtr->topics.SetBeacon(topicInfo.first, nullptr);
    }
  }

  // Unsubscribe from all the topics.
  for (auto topic : this->topicsSubscribed)
    this->Unsubscribe(topic);
}

//////////////////////////////////////////////////
void transport::Node::Advertise(const std::string &_topic)
{
  assert(_topic != "");

  zbeacon_t *topicBeacon = nullptr;

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  this->dataPtr->topics.SetAdvertisedByMe(_topic, true);

  if (!this->dataPtr->topics.GetBeacon(_topic, &topicBeacon))
  {
    // Create a new beacon for the topic.
    topicBeacon = zbeacon_new(this->dataPtr->ctx, this->dataPtr->bcastPort);
    this->dataPtr->topics.SetBeacon(_topic, topicBeacon);

    // Create the beacon content.
    transport::Header header(transport::Version, this->dataPtr->guid, _topic,
                             transport::AdvType, 0);
    transport::AdvMsg advMsg(header, this->dataPtr->myAddress);
    std::vector<char> buffer(advMsg.GetMsgLength());
    advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

    zbeacon_set_interval(topicBeacon, 2000);

    // Start publishing the ADVERTISE message periodically.
    zbeacon_publish(topicBeacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                    advMsg.GetMsgLength());
  }
}

//////////////////////////////////////////////////
void transport::Node::Unadvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  this->dataPtr->topics.SetAdvertisedByMe(_topic, false);

  // Stop broadcasting the beacon for this topic.
  zbeacon_t *topicBeacon = nullptr;
  if (this->dataPtr->topics.GetBeacon(_topic, &topicBeacon))
  {
    // Destroy the beacon.
    zbeacon_silence(topicBeacon);
    zbeacon_destroy(&topicBeacon);
    this->dataPtr->topics.SetBeacon(_topic, nullptr);
  }
}

//////////////////////////////////////////////////
int transport::Node::Publish(const std::string &_topic,
                             const transport::ProtoMsg &_msg)
{
  assert(_topic != "");

  if (!this->dataPtr->topics.AdvertisedByMe(_topic))
    return -1;

  // Local subscribers
  transport::ISubscriptionHandler_M handlers;
  this->dataPtr->topics.GetSubscriptionHandlers(_topic, handlers);
  for (auto handler : handlers)
  {
    transport::ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;
    if (subscriptionHandlerPtr)
      subscriptionHandlerPtr->RunLocalCallback(_topic, _msg);
    else
      std::cerr << "Subscription handler is NULL" << std::endl;
  }

  // Remote subscribers
  // if (this->dataPtr->topics.HasRemoteSubscribers(_topic))
  // {
    std::string data;
    _msg.SerializeToString(&data);
    if (this->dataPtr->Publish(_topic, data) != 0)
      return -1;
  // }

  return 0;
}

//////////////////////////////////////////////////
void transport::Node::Unsubscribe(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  if (this->dataPtr->verbose)
    std::cout << "\nUnsubscribe (" << _topic << ")\n";

  this->dataPtr->topics.RemoveSubscriptionHandler(_topic, this->nodeUuidStr);

  // Remove the topic from the list of subscribed topics in this node.
  this->topicsSubscribed.resize(
    std::remove(this->topicsSubscribed.begin(), this->topicsSubscribed.end(),
      _topic) - this->topicsSubscribed.begin());

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->topics.Subscribed(_topic))
  {
    this->dataPtr->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, _topic.data(), _topic.size());
  }
}
