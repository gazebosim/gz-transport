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
Discovery::Discovery(const uuid_t &_pUuid, bool _verbose)
: dataPtr(new DiscoveryPrivate(_pUuid, _verbose))
{
}

//////////////////////////////////////////////////
Discovery::~Discovery()
{
}

//////////////////////////////////////////////////
void Discovery::Advertise(const AdvertiseType &_advType,
  const std::string &_topic, const std::string &_addr, const std::string &_ctrl,
  const std::string &_nUuid, const Scope &_scope)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Add the addressing information (local node).
  this->dataPtr->info.AddAddress(_topic, _addr, _ctrl, this->dataPtr->pUuidStr,
    _nUuid, _scope);

  // Do not advertise a message outside the process.
  if (_scope == Scope::Process)
    return;

  this->dataPtr->NewBeacon(_advType, _topic, _nUuid);

  // Broadcast my topic information.
  // this->dataPtr->SendMsg(AdvType, _topic, _addr, _ctrl, _nUuid, _scope);
}

//////////////////////////////////////////////////
void Discovery::Discover(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Broadcast a discovery request for this topic.
  this->dataPtr->SendMsg(SubType, _topic, "", "", "", Scope::All);

  // I do not have information about this topic.
  if (!this->dataPtr->info.HasTopic(_topic))
  {
    // Add the topic to the unknown topic list if it was not before.
    /*if (std::find(this->dataPtr->unknownTopics.begin(),
        this->dataPtr->unknownTopics.end(), _topic) ==
          this->dataPtr->unknownTopics.end())
    {
      this->dataPtr->unknownTopics.push_back(_topic);
    }*/
  }
  // I have information stored for this topic.
  else if (this->dataPtr->connectionCb)
  {
    Addresses_M addresses;
    if (this->dataPtr->info.GetAddresses(_topic, addresses))
    {
      for (auto proc : addresses)
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
}

//////////////////////////////////////////////////
bool Discovery::GetTopicAddresses(const std::string &_topic,
                                  Addresses_M &_addresses)
{
  return this->dataPtr->info.GetAddresses(_topic, _addresses);
}

//////////////////////////////////////////////////
void Discovery::Unadvertise(const std::string &_topic,
                            const std::string &_nUuid)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  // Don't do anything if the topic is not advertised by any of my nodes.
  Address_t info;
  if (!this->dataPtr->info.GetAddress(_topic, this->dataPtr->pUuidStr,
        _nUuid, info))
    return;

  // Remove the topic information.
  this->dataPtr->info.DelAddressByNode(_topic, this->dataPtr->pUuidStr, _nUuid);

  // Do not advertise a message outside the process.
  if (info.scope == Scope::Process)
    return;

  // Send the UNADVERTISE message.
  this->dataPtr->SendMsg(UnadvType, _topic, info.addr, info.ctrl,
    _nUuid, info.scope);

  // Remove the beacon for this topic in this node.
  this->dataPtr->DelBeacon(_topic, _nUuid);
}

//////////////////////////////////////////////////
std::string Discovery::GetHostAddr() const
{
  return this->dataPtr->GetHostAddr();
}

//////////////////////////////////////////////////
unsigned int Discovery::GetActivityInterval() const
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->activityInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetHeartbitInterval() const
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->heartbitInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetAdvertiseInterval() const
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->advertiseInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetSilenceInterval() const
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->silenceInterval;
}

//////////////////////////////////////////////////
void Discovery::SetActivityInterval(const unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->activityInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetHeartbitInterval(const unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->heartbitInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetAdvertiseInterval(const unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->advertiseInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetSilenceInterval(const unsigned int _ms)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->silenceInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetConnectionsCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetDisconnectionsCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetConnectionsSrvCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionSrvCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetDisconnectionsSrvCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionSrvCb = _cb;
}

//////////////////////////////////////////////////
bool Discovery::Interrupted()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->exit;
}
