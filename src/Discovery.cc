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
#include <algorithm>
#include <mutex>
#include <string>
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

//////////////////////////////////////////////////
transport::Discovery::Discovery(const uuid_t &_procUuid, bool _verbose)
: dataPtr(new DiscoveryPrivate(_procUuid, _verbose))
{
}

//////////////////////////////////////////////////
transport::Discovery::~Discovery()
{
}

//////////////////////////////////////////////////
void transport::Discovery::Advertise(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Don't do anything if the topic is already advertised.
  if (this->dataPtr->AdvertisedByMe(_topic))
    return;

  // Add the addressing information.
  this->dataPtr->info[_topic] = transport::DiscoveryInfo(_addr, _ctrl,
    this->dataPtr->uuidStr);

  this->dataPtr->SendAdvertiseMsg(transport::AdvType, _topic);
}

//////////////////////////////////////////////////
void transport::Discovery::Discover(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  if (this->dataPtr->info.find(_topic) == this->dataPtr->info.end())
  {
    this->dataPtr->SendSubscribeMsg(transport::SubType, _topic);

    // Add the topic to the unknown topic list if it was not before.
    if (std::find(this->dataPtr->unknownTopics.begin(),
        this->dataPtr->unknownTopics.end(), _topic) ==
          this->dataPtr->unknownTopics.end())
    {
      this->dataPtr->unknownTopics.push_back(_topic);
    }
  }
  else if (this->dataPtr->connectionCb)
  {
    transport::DiscoveryInfo topicInfo = this->dataPtr->info[_topic];
    this->dataPtr->connectionCb(_topic, std::get<0>(topicInfo),
      std::get<1>(topicInfo), std::get<2>(topicInfo));
  }
}

//////////////////////////////////////////////////
void transport::Discovery::Unadvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Don't do anything if the topic is not advertised by me.
  if (!this->dataPtr->AdvertisedByMe(_topic))
    return;

  // Send the UNADVERTISE message.
  this->dataPtr->SendAdvertiseMsg(transport::UnadvType, _topic);

  // Remove the topic information.
  this->dataPtr->info.erase(_topic);
}

//////////////////////////////////////////////////
unsigned int transport::Discovery::GetMaxSilenceInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->silenceInterval;
}

//////////////////////////////////////////////////
unsigned int transport::Discovery::GetActivityInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->activityInterval;
}

//////////////////////////////////////////////////
unsigned int transport::Discovery::GetSubscriptionInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->subscriptionInterval;
}

//////////////////////////////////////////////////
unsigned int transport::Discovery::GetHelloInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->helloInterval;
}

//////////////////////////////////////////////////
void transport::Discovery::SetMaxSilenceInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->silenceInterval = _ms;
}

//////////////////////////////////////////////////
void transport::Discovery::SetActivityInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->activityInterval = _ms;
}

//////////////////////////////////////////////////
void transport::Discovery::SetSubscriptionInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->subscriptionInterval = _ms;
}

//////////////////////////////////////////////////
void transport::Discovery::SetHelloInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->helloInterval = _ms;
}

//////////////////////////////////////////////////
void transport::Discovery::SetConnectionsCb(
  const transport::DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  if (_cb != nullptr)
    this->dataPtr->connectionCb = _cb;
}

//////////////////////////////////////////////////
void transport::Discovery::SetDisconnectionsCb(
  const transport::DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  if (_cb != nullptr)
    this->dataPtr->disconnectionCb = _cb;
}
