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
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/ReqStorage.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
ReqStorage::ReqStorage()
{
}

//////////////////////////////////////////////////
ReqStorage::~ReqStorage()
{
}

//////////////////////////////////////////////////
void ReqStorage::GetReqHandlers(
  const std::string &_topic, IReqHandler_M &_handlers)
{
  if (this->requests.find(_topic) != this->requests.end())
    _handlers = this->requests[_topic];
}

//////////////////////////////////////////////////
void ReqStorage::AddReqHandler(const std::string &_topic,
  const std::string &_nodeUuid,
  const std::shared_ptr<IReqHandler> &_handler)
{
  // Create the topic entry.
  if (this->requests.find(_topic) == this->requests.end())
    this->requests[_topic] = {};

  // Create the Node UUID entry.
  if (this->requests[_topic].find(_nUuid) == this->requests[_topic].end())
    this->requests[_topic].insert(std::make_pair(_nodeUuid, nullptr));

  // Add/Replace the Req handler.
  this->requests[_topic][_nodeUuid].insert(
    std::make_pair(_handler->GetReqUuid(), _handler));
}

//////////////////////////////////////////////////
bool ReqStorage::Requested(const std::string &_topic)
{
  if (this->requests.find(_topic) == this->requests.end())
    return false;

  return !this->requests[_topic].empty();
}

//////////////////////////////////////////////////
void ReqStorage::RemoveReqHandler(const std::string &_topic,
  const std::string &_nUuid, const std::string, &_reqUuid)
{
  if (this->requests.find(_topic) != this->requests.end())
  {
    if (this->requests[_topic].find(_nUuid) != this->requests[_topic].end())
    {
      this->requests[_topic][_nUuid].erase(_reqUuid);
      if (this->requests[_topic][_nUuid].empty())
        this->requests[_topic].erase(_nUuid);
      if (this->requests[_topic].empty())
        this->requests.erase(_topic);
    }
  }
}
