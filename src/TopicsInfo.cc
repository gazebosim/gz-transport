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
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "ignition/transport/TopicsInfo.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
TopicInfo::TopicInfo()
  : advertisedByMe(false),
    requested(false),
    reqCb(nullptr),
    repCb(nullptr),
    beacon(nullptr)
{
}

//////////////////////////////////////////////////
TopicInfo::~TopicInfo()
{
}

//////////////////////////////////////////////////
TopicsInfo::TopicsInfo()
{
}

//////////////////////////////////////////////////
TopicsInfo::~TopicsInfo()
{
}

//////////////////////////////////////////////////
bool TopicsInfo::HasTopic(const std::string &_topic)
{
  return this->topicsInfo.find(_topic) != this->topicsInfo.end();
}


//////////////////////////////////////////////////
bool TopicsInfo::GetAdvAddresses(const std::string &_topic,
  Addresses_M &_addresses)
{
  if (!this->HasTopic(_topic))
    return false;

  _addresses = topicsInfo[_topic]->addresses;
  return true;
}

//////////////////////////////////////////////////
bool TopicsInfo::GetInfo(const std::string &_topic,
                         const std::string &_nUuid,
                         Address_t &_info)
{
  if (!this->HasTopic(_topic))
    return false;

  auto &m = this->topicsInfo[_topic]->addresses;

  for (auto proc : m)
  {
    auto &v = proc.second;
    auto found = std::find_if(v.begin(), v.end(),
      [&](const Address_t &_addrInfo)
      {
        return _addrInfo.nUuid == _nUuid;
      });
    // Address found!
    if (found != v.end())
    {
      _info = *found;
      return true;
    }
  }

  return false;
}

//////////////////////////////////////////////////
void TopicsInfo::ShowInfo()
{
  std::cout << "Topic info" << std::endl;
  for (auto topic : this->topicsInfo)
  {
    std::cout << "Topic: " << topic.first << std::endl;
    auto &m = topic.second->addresses;
    for (auto proc : m)
    {
      std::cout << "\tProc. UUID: " << proc.first << std::endl;
      auto &v = proc.second;
      for (auto info : v)
      {
        std::cout << "\t\tAddr:" << info.addr << std::endl;
        std::cout << "\t\tCtrl:" << info.ctrl << std::endl;
        std::cout << "\t\tNUUID:" << info.nUuid << std::endl;
      }
    }
  }
}

//////////////////////////////////////////////////
bool TopicsInfo::HasAdvAddress(const std::string &_topic,
  const std::string &_addr)
{
  if (!this->HasTopic(_topic))
    return false;

  auto &m = this->topicsInfo[_topic]->addresses;
  for (auto proc : m)
  {
    auto &v = proc.second;
    auto found = std::find_if(v.begin(), v.end(),
      [&](const Address_t &_addrInfo)
      {
        return _addrInfo.addr == _addr;
      });
    // Address found!
    if (found != v.end())
      return true;
  }

  return false;
}

//////////////////////////////////////////////////
bool TopicsInfo::Subscribed(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscriptionHandlers.size() > 0;
}

//////////////////////////////////////////////////
bool TopicsInfo::AdvertisedByMe(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->advertisedByMe;
}

//////////////////////////////////////////////////
bool TopicsInfo::Requested(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->requested;
}

//////////////////////////////////////////////////
bool TopicsInfo::GetBeacon(const std::string &_topic, zbeacon_t **_beacon)
{
  if (!this->HasTopic(_topic))
    return false;

  *_beacon = this->topicsInfo[_topic]->beacon;
  return *_beacon != nullptr;
}

//////////////////////////////////////////////////
bool TopicsInfo::GetReqCallback(const std::string &_topic, ReqCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->reqCb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool TopicsInfo::GetRepCallback(const std::string &_topic, RepCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->repCb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool TopicsInfo::PendingReqs(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return !this->topicsInfo[_topic]->pendingReqs.empty();
}

//////////////////////////////////////////////////
void TopicsInfo::AddAdvAddress(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const Scope &_scope)
{
  this->CheckAndCreate(_topic);

  // Check if the process uuid exists.
  auto &m = this->topicsInfo[_topic]->addresses;
  if (m.find(_pUuid) != m.end())
  {
    // Check that the structure {_addr, _ctrl, nUuid, scope} does not exist.
    auto &v = m[_pUuid];
    auto found = std::find_if(v.begin(), v.end(),
      [&](const Address_t &_addrInfo)
      {
        return _addrInfo.addr == _addr;
      });

    // _addr was already existing, just exit.
    if (found != v.end())
      return;
  }

  // Add a new address.
  m[_pUuid].push_back({_addr, _ctrl, _nUuid, _scope});
}

//////////////////////////////////////////////////
void TopicsInfo::DelAdvAddress(const std::string &/*_topic*/,
  const std::string &_addr, const std::string &_pUuid)
{
  for (auto topicInfo : this->topicsInfo)
  {
    auto &m = topicInfo.second;
    for (auto it = m->addresses.begin(); it != m->addresses.end();)
    {
      auto &v = it->second;
      v.erase(std::remove_if(v.begin(), v.end(),
        [&](const Address_t &_addrInfo)
        {
          return _addrInfo.addr == _addr;
        }),
        v.end());

      if (v.empty() || it->first == _pUuid)
        m->addresses.erase(it++);
      else
        ++it;
    }
  }
}

//////////////////////////////////////////////////
void TopicsInfo::DelAdvAddressByNode(const std::string &_nUuid)
{
  for (auto topicInfo : this->topicsInfo)
  {
    auto &m = topicInfo.second;
    for (auto it = m->addresses.begin(); it != m->addresses.end();)
    {
      auto &v = it->second;
      v.erase(std::remove_if(v.begin(), v.end(),
        [&](const Address_t &_addrInfo)
        {
          return _addrInfo.nUuid == _nUuid;
        }),
        v.end());

      if (v.empty())
        m->addresses.erase(it++);
      else
        ++it;
    }
  }
}

//////////////////////////////////////////////////
void TopicsInfo::SetRequested(const std::string &_topic, const bool _value)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->requested = _value;
}

//////////////////////////////////////////////////
void TopicsInfo::SetAdvertisedByMe(const std::string &_topic, const bool _value)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->advertisedByMe = _value;
}

//////////////////////////////////////////////////
void TopicsInfo::SetBeacon(const std::string &_topic, zbeacon_t *_beacon)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->beacon = _beacon;
}

//////////////////////////////////////////////////
void TopicsInfo::SetReqCallback(const std::string &_topic,
  const ReqCallback &_cb)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->reqCb = _cb;
}

//////////////////////////////////////////////////
void TopicsInfo::SetRepCallback(const std::string &_topic,
  const RepCallback &_cb)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->repCb = _cb;
}

//////////////////////////////////////////////////
void TopicsInfo::AddReq(const std::string &_topic, const std::string &_data)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->pendingReqs.push_back(_data);
}

//////////////////////////////////////////////////
bool TopicsInfo::DelReq(const std::string &_topic, std::string &_data)
{
  if (!this->HasTopic(_topic))
    return false;

  if (this->topicsInfo[_topic]->pendingReqs.empty())
    return false;

  _data = this->topicsInfo[_topic]->pendingReqs.front();
  this->topicsInfo[_topic]->pendingReqs.pop_front();
  return true;
}

//////////////////////////////////////////////////
Topics_M& TopicsInfo::GetTopicsInfo()
{
  return this->topicsInfo;
}

//////////////////////////////////////////////////
void TopicsInfo::AddRemoteSubscriber(const std::string &_topic,
  const std::string &_procUuid, const std::string &_nodeUuid)
{
  this->CheckAndCreate(_topic);

  auto &m = this->topicsInfo[_topic]->subscribers;
  // The process UUID does not exist yet.
  if (m.find(_procUuid) == m.end())
    m[_procUuid] = {};

  // Add the UUID if were not existing before.
  if (std::find(m[_procUuid].begin(), m[_procUuid].end(), _nodeUuid) ==
      m[_procUuid].end())
  {
    m[_procUuid].push_back(_nodeUuid);
  }
}

//////////////////////////////////////////////////
bool TopicsInfo::HasRemoteSubscribers(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return !this->topicsInfo[_topic]->subscribers.empty();
}

//////////////////////////////////////////////////
void TopicsInfo::DelRemoteSubscriber(const std::string &/*_topic*/,
  const std::string &_procUuid, const std::string &_nodeUuid)
{
  for (auto topicInfo : this->topicsInfo)
  {
    for (auto it = topicInfo.second->subscribers.begin();
         it != topicInfo.second->subscribers.end();)
    {
      auto &v = it->second;
      v.erase(std::remove(v.begin(), v.end(), _nodeUuid), v.end());

      if (v.empty() || it->first == _procUuid)
        topicInfo.second->subscribers.erase(it++);
      else
        ++it;
    }
  }
}

//////////////////////////////////////////////////
void TopicsInfo::GetSubscriptionHandlers(
  const std::string &_topic, ISubscriptionHandler_M &_handlers)
{
  if (this->HasTopic(_topic))
    _handlers = this->topicsInfo[_topic]->subscriptionHandlers;
}

//////////////////////////////////////////////////
void TopicsInfo::AddSubscriptionHandler(const std::string &_topic,
  const std::string &_nodeUuid,
  const std::shared_ptr<ISubscriptionHandler> &_msgPtr)
{
  this->CheckAndCreate(_topic);

  if (!this->HasSubscriptionHandler(_topic, _nodeUuid))
  {
    this->topicsInfo[_topic]->subscriptionHandlers.insert(
      std::make_pair(_nodeUuid, nullptr));
  }

  this->topicsInfo[_topic]->subscriptionHandlers[_nodeUuid] = _msgPtr;
}

//////////////////////////////////////////////////
void TopicsInfo::RemoveSubscriptionHandler(const std::string &_topic,
                                           const std::string &_nodeUuid)
{
  if (this->HasTopic(_topic))
    this->topicsInfo[_topic]->subscriptionHandlers.erase(_nodeUuid);
}

//////////////////////////////////////////////////
bool TopicsInfo::HasSubscriptionHandler(const std::string &_topic,
                                        const std::string &_nodeUuid)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscriptionHandlers.find(_nodeUuid) !=
      this->topicsInfo[_topic]->subscriptionHandlers.end();
}

//////////////////////////////////////////////////
void TopicsInfo::CheckAndCreate(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
  {
    this->topicsInfo.insert(
      make_pair(_topic, std::unique_ptr<TopicInfo>(new TopicInfo())));
  }
}
