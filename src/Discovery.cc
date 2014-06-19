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
#include <mutex>
#include <string>
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Discovery::Discovery(const std::string &_pUuid, bool _verbose)
: dataPtr(new DiscoveryPrivate(_pUuid, _verbose))
{
}

//////////////////////////////////////////////////
void Discovery::AdvertiseMsg(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_nUuid,
  const Scope &_scope)
{
  this->dataPtr->Advertise(MsgType::Msg, _topic, _addr, _ctrl, _nUuid, _scope);
}

//////////////////////////////////////////////////
void Discovery::AdvertiseSrvCall(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_nUuid,
  const Scope &_scope)
{
  this->dataPtr->Advertise(MsgType::Srv, _topic, _addr, _ctrl, _nUuid, _scope);
}

//////////////////////////////////////////////////
void Discovery::DiscoverMsg(const std::string &_topic)
{
  this->dataPtr->Discover(_topic, false);
}

//////////////////////////////////////////////////
void Discovery::DiscoverSrvCall(const std::string &_topic)
{
  this->dataPtr->Discover(_topic, true);
}

//////////////////////////////////////////////////
bool Discovery::GetTopicAddresses(const std::string &_topic,
                                  Addresses_M &_addresses)
{
  return this->dataPtr->info.GetAddresses(_topic, _addresses);
}

//////////////////////////////////////////////////
void Discovery::UnadvertiseMsg(const std::string &_topic,
                               const std::string &_nUuid)
{
  this->dataPtr->Unadvertise(MsgType::Msg, _topic, _nUuid);
}

//////////////////////////////////////////////////
void Discovery::UnadvertiseSrvCall(const std::string &_topic,
                                   const std::string &_nUuid)
{
  this->dataPtr->Unadvertise(MsgType::Srv, _topic, _nUuid);
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
bool Discovery::WasInterrupted()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->exit;
}
