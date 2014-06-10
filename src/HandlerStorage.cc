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
#include "ignition/transport/HandlerStorage.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
bool HandlerStorage::HasHandlersForTopic(const std::string &_topic)
{
  if (this->data.find(_topic) == this->data.end())
    return false;

  return !this->data[_topic].empty();
}

//////////////////////////////////////////////////
bool HandlerStorage::HasHandlersForNode(const std::string &_topic,
  const std::string &_nUuid)
{
  if (this->data.find(_topic) == this->data.end())
    return false;

  return this->data[_topic].find(_nUuid) != this->data[_topic].end();
}

//////////////////////////////////////////////////
void HandlerStorage::RemoveHandler(const std::string &_topic,
  const std::string &_nUuid, const std::string &_reqUuid)
{
  if (this->data.find(_topic) != this->data.end())
  {
    if (this->data[_topic].find(_nUuid) != this->data[_topic].end())
    {
      this->data[_topic][_nUuid].erase(_reqUuid);
      if (this->data[_topic][_nUuid].empty())
        this->data[_topic].erase(_nUuid);
      if (this->data[_topic].empty())
        this->data.erase(_topic);
    }
  }
}

//////////////////////////////////////////////////
void HandlerStorage::RemoveHandlersForNode(const std::string &_topic,
  const std::string &_nUuid)
{
  if (this->data.find(_topic) != this->data.end())
  {
    this->data[_topic].erase(_nUuid);
    if (this->data[_topic].empty())
      this->data.erase(_topic);
  }
}
