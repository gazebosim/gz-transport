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
bool TopicUtils::IsValidNamespace(const std::string &_ns)
{
  // "/" is not valid.
  if (_ns == "/")
    return false;

  // If the topic name has a '~' is not valid.
  if (_ns.find("~") != std::string::npos)
    return false;

  // If the topic name has a white space is not valid.
  if (_ns.find(" ") != std::string::npos)
    return false;

  // It is not allowed to have two consecutive slashes.
  if (_ns.find("//") != std::string::npos)
    return false;

  // It the topic name has a '@' is not valid.
  if (_ns.find("@") != std::string::npos)
    return false;

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::IsValidTopic(const std::string &_topic)
{
  return IsValidNamespace(_topic) && !_topic.empty();
}

//////////////////////////////////////////////////
bool TopicUtils::FullyQualifiedMsgName(const std::string &_partition,
  const std::string &_ns, const std::string &_topic, std::string &_name)
{
  return TopicUtils::FullyQualifiedName(_partition, _ns, _topic, _name);
}

//////////////////////////////////////////////////
bool TopicUtils::FullyQualifiedSrvName(const std::string &_partition,
  const std::string &_ns, const std::string &_topic, std::string &_name)
{
  return TopicUtils::FullyQualifiedName(_partition, _ns,
    _topic, _name, true);
}

//////////////////////////////////////////////////
bool TopicUtils::PartitionFromName(const std::string &_name,
  std::string &_partition)
{
  // Get partition from fully qualified name.
  auto topicName = _name;

  auto delimiter = topicName.find_first_of("@");
  // Error - no special part (requires at least two '@').
  if (delimiter == std::string::npos)
    return false;

  // Remove first '@'.
  topicName.erase(0, delimiter + 1);
  // Find where partition part ends.
  delimiter = topicName.find_first_of("@");
  // Error - no special part (requires at least two '@').
  if (delimiter == std::string::npos)
    return false;

  // Get partition.
  _partition = topicName.substr(0, delimiter);

  return TopicUtils::IsValidNamespace(_partition);
}

//////////////////////////////////////////////////
bool TopicUtils::TypeFromName(const std::string &_name, std::string &_type)
{
  // Get publisher type from fully qualified name.
  auto topicName = _name;
  size_t delimiter;

  for (auto i = 0; i < 2; ++i)
  {
    delimiter = topicName.find_first_of("@");
    // Error - no special part (requires at least three '@').
    if (delimiter == std::string::npos)
      return false;

    // Remove until '@'.
    topicName.erase(0, delimiter + 1);
  }

  // Find where type part ends.
  delimiter = topicName.find_first_of("@");
  // Error - no special part (requires at least three '@').
  if (delimiter == std::string::npos)
    return false;

  // Get type.
  _type = topicName.substr(0, delimiter);

  return (_type == "msg" || _type == "srv");
}

//////////////////////////////////////////////////
bool TopicUtils::FullyQualifiedName(const std::string &_partition,
  const std::string &_ns, const std::string &_topic, std::string &_name,
  const bool& _isService)
{
  // Sanity check, first things first.
  if (!IsValidNamespace(_partition) || !IsValidNamespace(_ns) ||
      !IsValidTopic(_topic))
  {
    return false;
  }

  std::string partition = _partition;
  std::string ns = _ns;
  std::string topic = _topic;

  // If partition is not empty and does not start with slash, add it.
  if (!partition.empty() && partition.front() != '/')
    partition.insert(0, 1, '/');

  // If the partition contains a trailing slash, remove it.
  if (!partition.empty() && partition.back() == '/')
    partition.pop_back();

  // If the namespace does not contain a trailing slash, append it.
  if (ns.empty() || ns.back() != '/')
    ns.push_back('/');

  // If the namespace does not start with slash, add it.
  if (ns.empty() || ns.front() != '/')
    ns.insert(0, 1, '/');

  // If the topic ends in "/", remove it.
  if (!topic.empty() && topic.back() == '/')
    topic.pop_back();

  // If the topic does starts with '/' is considered an absolute topic and the
  // namespace will not be prefixed.
  if (!topic.empty() && topic.front() == '/')
    _name = topic;
  else
    _name = ns + topic;

  // Add the partition prefix.
  _name.insert(0, "@" + partition + "@");

  if (_isService)
    _name.insert(_name.find_last_of("@") + 1, "srv@");
  else
    _name.insert(_name.find_last_of("@") + 1, "msg@");

  return true;
}
