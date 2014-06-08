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

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
RepStorage::RepStorage()
{
}

//////////////////////////////////////////////////
RepStorage::~RepStorage()
{
}

//////////////////////////////////////////////////
void RepStorage::GetRepHandlers(
  const std::string &_topic, IRepHandler_M &_handlers)
{
  if (this->responsers.find(_topic) != this->responsers.end())
    _handlers = this->responsers[_topic];
}

//////////////////////////////////////////////////
void RepStorage::AddRepHandler(const std::string &_topic,
  const std::string &_nodeUuid,
  const std::shared_ptr<IRepHandler> &_handler)
{
  // Create the topic entry.
  if (this->responsers.find(_topic) == this->responsers.end())
    this->responsers[_topic] = {};

  // Create the Node UUID entry.
  if (!this->HasRepHandler(_topic, _nodeUuid))
    this->responsers[_topic].insert(std::make_pair(_nodeUuid, nullptr));

  // Add/Replace the Rep handler.
  this->responsers[_topic][_nodeUuid] = _handler;
}

//////////////////////////////////////////////////
bool RepStorage::Advertised(const std::string &_topic)
{
  if (this->responsers.find(_topic) == this->responsers.end())
    return false;

  return !this->responsers[_topic].empty();
}

//////////////////////////////////////////////////
void RepStorage::RemoveRepHandler(const std::string &_topic,
  const std::string &_nUuid)
{
  if (this->responsers.find(_topic) != this->responsers.end())
  {
    this->responsers[_topic].erase(_nUuid);
    if (this->responsers[_topic].empty())
      this->responsers.erase(_topic);
  }
}

//////////////////////////////////////////////////
bool RepStorage::HasRepHandler(const std::string &_topic,
  const std::string &_nUuid)
{
  if (this->responsers.find(_topic) == this->responsers.end())
    return false;

  return this->responsers[_topic].find(_nUuid) !=
    this->responsers[_topic].end();
}
