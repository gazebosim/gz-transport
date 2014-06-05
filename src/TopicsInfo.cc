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
AddressInfo::AddressInfo()
{
}

//////////////////////////////////////////////////
AddressInfo::~AddressInfo()
{
}

//////////////////////////////////////////////////
bool AddressInfo::AddAddress(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const Scope &_scope)
{
  // The topic does not exist.
  if (this->data.find(_topic) == this->data.end())
    this->data[_topic] = {};

  // Check if the process uuid exists.
  auto &m = this->data[_topic];
  if (m.find(_pUuid) != m.end())
  {
    // Check that the structure {_addr, _ctrl, _nUuid, scope} does not exist.
    auto &v = m[_pUuid];
    auto found = std::find_if(v.begin(), v.end(),
      [&](const Address_t &_addrInfo)
      {
        return _addrInfo.addr == _addr && _addrInfo.nUuid == _nUuid;
      });

    // _addr was already existing, just exit.
    if (found != v.end())
      return false;
  }

  // Add a new address information entry.
  m[_pUuid].push_back({_addr, _ctrl, _nUuid, _scope});
  return true;
}

//////////////////////////////////////////////////
bool AddressInfo::HasTopic(const std::string &_topic)
{
  return this->data.find(_topic) != this->data.end();
}

//////////////////////////////////////////////////
bool AddressInfo::HasAddresses(const std::string &_topic,
  const std::string &_pUuid)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->data[_topic].find(_pUuid) != this->data[_topic].end();
}


//////////////////////////////////////////////////
bool AddressInfo::GetAddress(const std::string &_topic,
  const std::string &_pUuid, const std::string &_nUuid, Address_t &_info)
{
  // Topic not found.
  if (this->data.find(_topic) == this->data.end())
    return false;

  // m is pUUID->{addr, ctrl, nUuid, scope}.
  auto &m = this->data[_topic];

  // pUuid not found.
  if (m.find(_pUuid) == m.end())
    return false;

  // Vector of 0MQ known addresses for a given topic and pUuid.
  auto &v = m[_pUuid];
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

  return false;
}

//////////////////////////////////////////////////
bool AddressInfo::GetAddresses(const std::string &_topic, Addresses_M &_info)
{
  if (!this->HasTopic(_topic))
    return false;

  _info = this->data[_topic];
  return true;
}

//////////////////////////////////////////////////
void AddressInfo::DelAddressByNode(const std::string &_topic,
  const std::string &_pUuid, const std::string &_nUuid)
{
  // Iterate over all the topics.
  if (this->data.find(_topic) != this->data.end())
  {
    // m is pUUID->{addr, ctrl, nUuid, scope}.
    auto &m = this->data[_topic];

    // The pUuid exists.
    if (m.find(_pUuid) != m.end())
    {
      // Vector of 0MQ known addresses for a given topic and pUuid.
      auto &v = m[_pUuid];
      v.erase(std::remove_if(v.begin(), v.end(),
        [&](const Address_t &_addrInfo)
        {
          return _addrInfo.nUuid == _nUuid;
        }),
        v.end());

      if (v.empty())
        m.erase(_pUuid);

      if (m.empty())
        this->data.erase(_topic);
    }
  }
}

//////////////////////////////////////////////////
void AddressInfo::DelAddressesByProc(const std::string &_pUuid)
{
  // Iterate over all the topics.
  for (auto it = this->data.begin(); it != this->data.end();)
  {
    // m is pUUID->{addr, ctrl, nUuid, scope}.
    auto &m = it->second;
    m.erase(_pUuid);
    if (m.empty())
      this->data.erase(it++);
    else
      ++it;
  }
}

//////////////////////////////////////////////////
void AddressInfo::Print()
{
  std::cout << "---" << std::endl;
  for (auto &topic : this->data)
  {
    std::cout << "[" << topic.first << "]" << std::endl;
    auto &m = topic.second;
    for (auto &proc : m)
    {
      std::cout << "\tProc. UUID: " << proc.first << std::endl;
      auto &v = proc.second;
      for (auto &info : v)
      {
        std::cout << "\t\tAddr:" << info.addr << std::endl;
        std::cout << "\t\tCtrl:" << info.ctrl << std::endl;
        std::cout << "\t\tNUUID:" << info.nUuid << std::endl;
      }
    }
  }
}

//////////////////////////////////////////////////
TopicInfo::TopicInfo()
  : requested(false),
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
bool TopicsInfo::Subscribed(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscriptionHandlers.size() > 0;
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
void TopicsInfo::SetRequested(const std::string &_topic, const bool _value)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->requested = _value;
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

  // Add the UUID if was not existing before.
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
void TopicsInfo::DelRemoteSubscriberByNode(const std::string &_topic,
  const std::string &_pUuid, const std::string &_nUuid)
{
  // Iterate over all the topics.
  if (this->topicsInfo.find(_topic) != this->topicsInfo.end())
  {
    // m is pUUID->{addr, ctrl, nUuid, scope}.
    auto &m = this->topicsInfo[_topic]->subscribers;

    // The pUuid exists.
    if (m.find(_pUuid) != m.end())
    {
      // Vector of 0MQ known addresses for a given topic and pUuid.
      auto &v = m[_pUuid];
      v.erase(std::remove(v.begin(), v.end(), _nUuid), v.end());

      if (v.empty())
        m.erase(_pUuid);

      // if (m.empty())
      //  this->topicsInfo.erase(_topic);
    }
  }
}

//////////////////////////////////////////////////
void TopicsInfo::DelRemoteSubscriberByProc(const std::string &_pUuid)
{
  // Iterate over all the topics.
  for (auto it = this->topicsInfo.begin(); it != this->topicsInfo.end();)
  {
    // m is pUUID->{addr, ctrl, nUuid, scope}.
    auto &m = it->second->subscribers;
    m.erase(_pUuid);
    // if (m.empty())
    //   this->topicsInfo.erase(it++);
    // else
    ++it;
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
