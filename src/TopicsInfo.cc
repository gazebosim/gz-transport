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

//////////////////////////////////////////////////
transport::TopicInfo::TopicInfo()
  : advertisedByMe(false),
    requested(false),
    reqCb(nullptr),
    repCb(nullptr),
    beacon(nullptr)
{
}

//////////////////////////////////////////////////
transport::TopicInfo::~TopicInfo()
{
}

//////////////////////////////////////////////////
transport::TopicsInfo::TopicsInfo()
{
}

//////////////////////////////////////////////////
transport::TopicsInfo::~TopicsInfo()
{
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::HasTopic(const std::string &_topic)
{
  return this->topicsInfo.find(_topic) != this->topicsInfo.end();
}


//////////////////////////////////////////////////
bool transport::TopicsInfo::GetAdvAddresses(const std::string &_topic,
                                            transport::Addresses_M &_addresses)
{
  if (!this->HasTopic(_topic))
    return false;

  _addresses = topicsInfo[_topic]->addresses;
  return true;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::HasAdvAddress(const std::string &_topic,
                                          const std::string &_addr)
{
  if (!this->HasTopic(_topic))
    return false;

  auto &m = this->topicsInfo[_topic]->addresses;
  for (auto proc : m)
  {
    auto &v = proc.second;
    auto found = std::find_if(v.begin(), v.end(),
      [=](Address_t _addrInfo)
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
bool transport::TopicsInfo::Subscribed(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscriptionHandlers.size() > 0;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::AdvertisedByMe(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->advertisedByMe;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::Requested(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->requested;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::GetBeacon(const std::string &_topic,
                                      zbeacon_t **_beacon)
{
  if (!this->HasTopic(_topic))
  {
    return false;
  }

  *_beacon = this->topicsInfo[_topic]->beacon;
  return *_beacon != nullptr;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::GetReqCallback(const std::string &_topic,
                                           transport::ReqCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->reqCb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::GetRepCallback(const std::string &_topic,
                                           transport::RepCallback &_cb)
{
  if (!this->HasTopic(_topic))
    return false;

  _cb = this->topicsInfo[_topic]->repCb;
  return _cb != nullptr;
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::PendingReqs(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return !this->topicsInfo[_topic]->pendingReqs.empty();
}

//////////////////////////////////////////////////
void transport::TopicsInfo::AddAdvAddress(const std::string &_topic,
                                          const std::string &_addr,
                                          const std::string &_ctrl,
                                          const std::string &_uuid)
{
  this->CheckAndCreate(_topic);

  // Check if the process uuid exists.
  auto &m = this->topicsInfo[_topic]->addresses;
  if (m.find(_uuid) != m.end())
  {
    // Check that the {_addr, _ctrl} does not exist.
    auto &v = m[_uuid];
    auto found = std::find_if(v.begin(), v.end(),
      [=](Address_t _addrInfo)
      {
        return _addrInfo.addr == _addr;
      });

    // _addr was already existing, just exit.
    if (found != v.end())
      return;
  }

  // Add a new address.
  m[_uuid].push_back({_addr, _ctrl});
}

//////////////////////////////////////////////////
void transport::TopicsInfo::DelAdvAddress(const std::string &/*_topic*/,
                                          const std::string &_addr,
                                          const std::string &_uuid)
{
  for (auto topicInfo : this->topicsInfo)
  {
    auto m = topicInfo.second;
    for (auto it = m->addresses.begin();
         it != m->addresses.end();)
    {
      auto &v = it->second;
      v.erase(std::remove_if(v.begin(), v.end(), [=](Address_t _addrInfo)
      {
        return _addrInfo.addr == _addr;
      }), v.end());

      it->second = v;

      if (v.empty() || it->first == _uuid)
        m->addresses.erase(it++);
      else
        ++it;
    }
  }
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetRequested(const std::string &_topic,
                                         const bool _value)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->requested = _value;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetAdvertisedByMe(const std::string &_topic,
                                              const bool _value)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->advertisedByMe = _value;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetBeacon(const std::string &_topic,
                                      zbeacon_t *_beacon)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->beacon = _beacon;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetReqCallback(const std::string &_topic,
                                           const transport::ReqCallback &_cb)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->reqCb = _cb;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::SetRepCallback(const std::string &_topic,
                                           const transport::RepCallback &_cb)
{
  this->CheckAndCreate(_topic);
  this->topicsInfo[_topic]->repCb = _cb;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::AddReq(const std::string &_topic,
                                   const std::string &_data)
{
  this->CheckAndCreate(_topic);

  this->topicsInfo[_topic]->pendingReqs.push_back(_data);
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::DelReq(const std::string &_topic,
                                   std::string &_data)
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
transport::Topics_M& transport::TopicsInfo::GetTopicsInfo()
{
  return this->topicsInfo;
}

//////////////////////////////////////////////////
void transport::TopicsInfo::AddRemoteSubscriber(const std::string &_topic,
                                                const std::string &_procUuid,
                                                const std::string &_nodeUuid)
{
  this->CheckAndCreate(_topic);

  // The process UUID does not exist yet.
  if (this->topicsInfo[_topic]->subscribers.find(_procUuid) ==
      this->topicsInfo[_topic]->subscribers.end())
  {
    this->topicsInfo[_topic]->subscribers[_procUuid] =
      std::vector<std::string>();
  }

  // Add the UUID if were not existing before.
  if (std::find(this->topicsInfo[_topic]->subscribers[_procUuid].begin(),
        this->topicsInfo[_topic]->subscribers[_procUuid].end(), _nodeUuid) ==
          this->topicsInfo[_topic]->subscribers[_procUuid].end())
  {
    this->topicsInfo[_topic]->subscribers[_procUuid].push_back(_nodeUuid);
  }
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::HasRemoteSubscribers(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
    return false;

  return !this->topicsInfo[_topic]->subscribers.empty();
}

//////////////////////////////////////////////////
void transport::TopicsInfo::DelRemoteSubscriber(const std::string &/*_topic*/,
                                                const std::string &_procUuid,
                                                const std::string &_nodeUuid)
{
  for (auto topicInfo : this->topicsInfo)
  {
    for (auto it = topicInfo.second->subscribers.begin();
         it != topicInfo.second->subscribers.end();)
    {
      std::vector<std::string> v = it->second;
      v.erase(std::remove(v.begin(), v.end(), _nodeUuid), v.end());
      it->second = v;

      if (v.empty() || it->first == _procUuid)
        topicInfo.second->subscribers.erase(it++);
      else
        ++it;
    }
  }
}

//////////////////////////////////////////////////
void transport::TopicsInfo::GetSubscriptionHandlers(
  const std::string &_topic, transport::ISubscriptionHandler_M &_handlers)
{
  if (this->HasTopic(_topic))
  {
    _handlers = this->topicsInfo[_topic]->subscriptionHandlers;
  }
}

//////////////////////////////////////////////////
void transport::TopicsInfo::AddSubscriptionHandler(const std::string &_topic,
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
void transport::TopicsInfo::RemoveSubscriptionHandler(const std::string &_topic,
                                                   const std::string &_nodeUuid)
{
  if (this->HasTopic(_topic))
    this->topicsInfo[_topic]->subscriptionHandlers.erase(_nodeUuid);
}

//////////////////////////////////////////////////
bool transport::TopicsInfo::HasSubscriptionHandler(const std::string &_topic,
                                                   const std::string &_nodeUuid)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->topicsInfo[_topic]->subscriptionHandlers.find(_nodeUuid) !=
      this->topicsInfo[_topic]->subscriptionHandlers.end();
}

//////////////////////////////////////////////////
void transport::TopicsInfo::CheckAndCreate(const std::string &_topic)
{
  if (!this->HasTopic(_topic))
  {
    this->topicsInfo.insert(
      make_pair(_topic, std::unique_ptr<TopicInfo>(new TopicInfo())));
  }
}
