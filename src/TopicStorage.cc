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
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
bool TopicStorage::AddAddress(const std::string &_topic,
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
bool TopicStorage::HasTopic(const std::string &_topic)
{
  return this->data.find(_topic) != this->data.end();
}

//////////////////////////////////////////////////
bool TopicStorage::HasAnyAddresses(const std::string &_topic,
  const std::string &_pUuid)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->data[_topic].find(_pUuid) != this->data[_topic].end();
}

//////////////////////////////////////////////////
bool TopicStorage::HasAddress(const std::string &_addr)
{
  for (auto &topic : this->data)
  {
    for (auto &proc : topic.second)
    {
      for (auto &info : proc.second)
      {
        if (info.addr == _addr)
          return true;
      }
    }
  }
  return false;
}


//////////////////////////////////////////////////
bool TopicStorage::GetAddress(const std::string &_topic,
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
bool TopicStorage::GetAddresses(const std::string &_topic, Addresses_M &_info)
{
  if (!this->HasTopic(_topic))
    return false;

  _info = this->data[_topic];
  return true;
}

//////////////////////////////////////////////////
void TopicStorage::DelAddressByNode(const std::string &_topic,
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
void TopicStorage::DelAddressesByProc(const std::string &_pUuid)
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
void TopicStorage::Print()
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
        std::cout << "\t\t* Addr:" << info.addr << std::endl;
        std::cout << "\t\t  Ctrl:" << info.ctrl << std::endl;
        std::cout << "\t\t  Node UUID:" << info.nUuid << std::endl;
        switch (info.scope)
        {
          case Scope::Process:
            std::cout << "\t\t  Scope: Process" << std::endl;
            break;
          case Scope::Host:
            std::cout << "\t\t  Scope: Host" << std::endl;
            break;
          case Scope::All:
            std::cout << "\t\t  Scope: All" << std::endl;
            break;
          default:
            std::cout << "\t\t  Scope: <Unknown>" << std::endl;
            break;
        }
      }
    }
  }
}
