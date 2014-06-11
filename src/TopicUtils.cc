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
#include "ignition/transport/TopicUtils.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
bool TopicUtils::IsValidTopic(const std::string &_topic)
{
  // The empty string is not valid.
  if (_topic == "")
    return false;

  // The topics '~', '/', '~/' are not valid.
  if (_topic == "~" || _topic == "/" || _topic == "~/")
    return false;

  // If the topic name has a white space is not valid.
  if (_topic.find(" ") != std::string::npos)
    return false;

  // It is not allowed to have two consecutive slashes.
  if (_topic.find("//") != std::string::npos)
    return false;

  // The symbol '~' is allowed to represent a 'relative' path but is only
  // allowed at the start of the topic name.
  auto pos = _topic.find("~");
  if (pos != std::string::npos && pos != 0)
  {
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::IsValidNamespace(const std::string &_ns)
{
  // The empty string is not valid.
  if (_ns == "")
    return false;

  // The character '~' is not valid.
  if (_ns.find("~") != std::string::npos)
    return false;

  // If the topic name has a white space is not valid.
  if (_ns.find(" ") != std::string::npos)
    return false;

  // It is not allowed to have two consecutive slashes.
  if (_ns.find("//") != std::string::npos)
    return false;

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::GetScopedName(const std::string &_ns,
  const std::string &_topic, std::string &_scoped)
{
  // Sanity check, first things first.
  if (!IsValidNamespace(_ns) || !IsValidTopic(_topic))
    return false;

  std::string ns = _ns;
  std::string topic = _topic;

  // If the namespace does not contain a trailing slash, append it.
  if (ns.back() != '/')
    ns.push_back('/');

  // If the namespace does not start with slash, add it.
  if (ns.front() != '/')
    ns.insert(0, 1, '/');

  // If the topic ends in "/", remove it.
  if (_topic.back() == '/')
    topic.pop_back();

  // If the topic starts with '~' is considered a relative path and the
  // namespace will be prefixed.
  if (topic.find("~/") == 0)
    _scoped = ns + topic.substr(2);
  else if (topic.find("~") == 0)
    _scoped = ns + topic.substr(1);
  else if (topic.front() != '/')
    _scoped = "/" + topic;
  else
    _scoped = topic;

  return true;
}
