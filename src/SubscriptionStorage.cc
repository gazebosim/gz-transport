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

#include <string>
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/SubscriptionStorage.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
SubscriptionStorage::SubscriptionStorage()
{
}

//////////////////////////////////////////////////
SubscriptionStorage::~SubscriptionStorage()
{
}

//////////////////////////////////////////////////
void SubscriptionStorage::GetSubscriptionHandlers(
  const std::string &_topic, ISubscriptionHandler_M &_handlers)
{
  if (this->subscribers.find(_topic) != this->subscribers.end())
    _handlers = this->subscribers[_topic];
}

//////////////////////////////////////////////////
void SubscriptionStorage::AddSubscriptionHandler(const std::string &_topic,
  const std::string &_nodeUuid,
  const std::shared_ptr<ISubscriptionHandler> &_msgPtr)
{
  // Create the topic entry.
  if (this->subscribers.find(_topic) == this->subscribers.end())
    this->subscribers[_topic] = {};

  // Create the Node UUID entry.
  if (!this->HasSubscriptionHandler(_topic, _nodeUuid))
    this->subscribers[_topic].insert(std::make_pair(_nodeUuid, nullptr));

  // Add/Replace the subscription handler.
  this->subscribers[_topic][_nodeUuid] = _msgPtr;
}

//////////////////////////////////////////////////
bool SubscriptionStorage::Subscribed(const std::string &_topic)
{
  if (this->subscribers.find(_topic) == this->subscribers.end())
    return false;

  return !this->subscribers[_topic].empty();
}

//////////////////////////////////////////////////
void SubscriptionStorage::RemoveSubscriptionHandler(const std::string &_topic,
  const std::string &_nodeUuid)
{
  if (this->subscribers.find(_topic) != this->subscribers.end())
  {
    this->subscribers[_topic].erase(_nodeUuid);
    if (this->subscribers[_topic].empty())
      this->subscribers.erase(_topic);
  }
}

//////////////////////////////////////////////////
bool SubscriptionStorage::HasSubscriptionHandler(const std::string &_topic,
  const std::string &_nodeUuid)
{
  if (this->subscribers.find(_topic) == this->subscribers.end())
    return false;

  return this->subscribers[_topic].find(_nodeUuid) !=
    this->subscribers[_topic].end();
}
