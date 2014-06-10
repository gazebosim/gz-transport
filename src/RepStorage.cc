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
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/RepStorage.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
bool RepStorage::GetHandlers(const std::string &_topic,
  IRepHandler_M &_handlers)
{
  if (this->responsers.find(_topic) == this->responsers.end())
    return false;

  _handlers = this->responsers[_topic];
  return true;
}

//////////////////////////////////////////////////
void RepStorage::AddHandler(const std::string &_topic,
  const std::string &_nUuid, const std::shared_ptr<IRepHandler> &_handler)
{
  // Create the topic entry.
  if (this->responsers.find(_topic) == this->responsers.end())
    this->responsers[_topic] = {};

  // Create the Node UUID entry.
  if (this->responsers[_topic].find(_nUuid) == this->responsers[_topic].end())
    this->responsers[_topic].insert(std::make_pair(_nUuid, nullptr));

  // Add/Replace the Rep handler.
  this->responsers[_topic][_nUuid] = _handler;
}

//////////////////////////////////////////////////
bool RepStorage::HasHandlerForTopic(const std::string &_topic)
{
  if (this->responsers.find(_topic) == this->responsers.end())
    return false;

  return !this->responsers[_topic].empty();
}

//////////////////////////////////////////////////
bool RepStorage::HasHandlerForNode(const std::string &_topic,
  const std::string &_nUuid)
{
  if (this->responsers.find(_topic) == this->responsers.end())
    return false;

  return this->responsers[_topic].find(_nUuid) !=
    this->responsers[_topic].end();
}

//////////////////////////////////////////////////
void RepStorage::RemoveHandler(const std::string &_topic,
  const std::string &_nUuid)
{
  if (this->responsers.find(_topic) != this->responsers.end())
  {
    this->responsers[_topic].erase(_nUuid);
    if (this->responsers[_topic].empty())
      this->responsers.erase(_topic);
  }
}
