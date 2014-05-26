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
using namespace transport;

//////////////////////////////////////////////////
Discovery::Discovery(const uuid_t &_procUuid, bool _verbose)
: dataPtr(new DiscoveryPrivate(_procUuid, _verbose))
{
}

//////////////////////////////////////////////////
Discovery::~Discovery()
{
}

//////////////////////////////////////////////////
void Discovery::Advertise(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Don't do anything if the topic is already advertised.
  if (this->dataPtr->AdvertisedByMe(_topic))
    return;

  // Add the addressing information.
  this->dataPtr->info[_topic] = DiscoveryInfo(_addr, _ctrl,
    this->dataPtr->uuidStr);

  // Broadcast my topic information.
  this->dataPtr->SendMsg(AdvType, _topic);
}

//////////////////////////////////////////////////
void Discovery::Discover(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // I do not have information about this topic.
  if (this->dataPtr->info.find(_topic) == this->dataPtr->info.end())
  {
    // Broadcast a discovery request for this topic.
    this->dataPtr->SendMsg(SubType, _topic);

    // Add the topic to the unknown topic list if it was not before.
    if (std::find(this->dataPtr->unknownTopics.begin(),
        this->dataPtr->unknownTopics.end(), _topic) ==
          this->dataPtr->unknownTopics.end())
    {
      this->dataPtr->unknownTopics.push_back(_topic);
    }
  }
  // I have information stored for this topic.
  else if (this->dataPtr->connectionCb)
  {
    // Execute the user's callback
    DiscoveryInfo topicInfo = this->dataPtr->info[_topic];
    this->dataPtr->connectionCb(_topic, std::get<Addr>(topicInfo),
      std::get<Ctrl>(topicInfo), std::get<Uuid>(topicInfo));
  }
}

//////////////////////////////////////////////////
void Discovery::Unadvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Don't do anything if the topic is not advertised by me.
  if (!this->dataPtr->AdvertisedByMe(_topic))
    return;

  // Send the UNADVERTISE message.
  this->dataPtr->SendMsg(UnadvType, _topic);

  // Remove the topic information.
  this->dataPtr->info.erase(_topic);
}

//////////////////////////////////////////////////
unsigned int Discovery::GetMaxSilenceInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->silenceInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetActivityInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->activityInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetSubscriptionInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->subscriptionInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetHelloInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->helloInterval;
}

//////////////////////////////////////////////////
void Discovery::SetMaxSilenceInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->silenceInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetActivityInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->activityInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetSubscriptionInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->subscriptionInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetHelloInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->helloInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetConnectionsCb(
  const DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetDisconnectionsCb(
  const DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionCb = _cb;
}
