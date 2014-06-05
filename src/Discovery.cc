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
#include <iostream>
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
void Discovery::Advertise(const std::string &_topic, const std::string &_addr,
                          const std::string &_ctrl, const std::string &_nUuid,
                          const Scope &_scope)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Add the addressing information (local node).
  this->dataPtr->AddTopicAddress(_topic, _addr, _ctrl,
    this->dataPtr->uuidStr, _nUuid, _scope);

  // Do not advertise a message outside the process.
  if (_scope == Scope::Process)
    return;

  // Broadcast my topic information.
  this->dataPtr->SendMsg(AdvType, _topic, _addr, _ctrl, _nUuid, _scope);
}

//////////////////////////////////////////////////
void Discovery::Discover(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Broadcast a discovery request for this topic.
  this->dataPtr->SendMsg(SubType, _topic, "", "", "", Scope::All);

  // I do not have information about this topic.
  if (this->dataPtr->info.find(_topic) == this->dataPtr->info.end())
  {
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
    for (auto proc : this->dataPtr->info[_topic])
    {
      for (auto node : proc.second)
      {
        // Execute the user's callback.
        this->dataPtr->connectionCb(_topic, node.addr, node.ctrl, proc.first,
          node.nUuid, node.scope);
      }
    }
  }
}

//////////////////////////////////////////////////
bool Discovery::GetTopicAddresses(const std::string &_topic,
                                  Addresses_M &_addresses)
{
  // Topic not found.
  if (this->dataPtr->info.find(_topic) == this->dataPtr->info.end())
    return false;

  _addresses = this->dataPtr->info[_topic];
  return true;
}

//////////////////////////////////////////////////
void Discovery::Unadvertise(const std::string &_topic,
                            const std::string &_nUuid)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Don't do anything if the topic is not advertised by any of my nodes.
  Address_t info;
  if (!this->dataPtr->GetTopicAddress(_topic, this->dataPtr->uuidStr,
        _nUuid, info))
    return;

  // Remove the topic information.
  this->dataPtr->DelTopicAddrByNode(_topic, this->dataPtr->uuidStr, _nUuid);

  // Do not advertise a message outside the process.
  if (info.scope == Scope::Process)
    return;

  // Send the UNADVERTISE message.
  this->dataPtr->SendMsg(UnadvType, _topic, info.addr, info.ctrl,
    _nUuid, info.scope);
}

//////////////////////////////////////////////////
std::string Discovery::GetHostAddr()
{
  return this->dataPtr->GetHostAddr();
}

//////////////////////////////////////////////////
unsigned int Discovery::GetActivityInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->activityInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetHeartbitInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->heartbitInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetRetransmissionInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->retransmissionInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetSilenceInterval()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->silenceInterval;
}

//////////////////////////////////////////////////
void Discovery::SetActivityInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->activityInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetHeartbitInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->heartbitInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetRetransmissionInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->retransmissionInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetSilenceInterval(unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->silenceInterval = _ms;
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

//////////////////////////////////////////////////
bool Discovery::Interrupted()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->exit;
}
