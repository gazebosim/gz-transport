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
bool TopicUtils::GetFullyQualifiedName(const std::string &_partition,
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
    _name.insert(_name.find_last_of("@")+1, "srv@");
  else
    _name.insert(_name.find_last_of("@")+1, "msg@");

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::GetFullyQualifiedMsgName(const std::string &_partition,
  const std::string &_ns, const std::string &_topic, std::string &_name)
{
  return TopicUtils::GetFullyQualifiedName(_partition, _ns, _topic, _name);
}

//////////////////////////////////////////////////
bool TopicUtils::GetFullyQualifiedSrvName(const std::string &_partition,
  const std::string &_ns, const std::string &_topic, std::string &_name)
{
  return TopicUtils::GetFullyQualifiedName(_partition, _ns,
    _topic, _name, true);
}

//////////////////////////////////////////////////
bool TopicUtils::GetPartitionFromName(const std::string &_name,
  std::string &_partition)
{
  // Get partition from fully qualified name
  auto topic_name = _name;

  auto first = topic_name.find_first_of("@");
  // ERROR - no special part (requires at least 2 '@')
  if (first == std::string::npos)
  {
    return false;
  }
  // remove first '@'
  topic_name.erase(0, first + 1);
  // find where partition part ends
  first = topic_name.find_first_of("@");
  // ERROR - no special part (requires at least 2 '@')
  if (first == std::string::npos)
  {
    return false;
  }
  // get partition
  _partition = topic_name.substr(0, first);

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::GetTypeFromName(const std::string &_name, std::string &_type)
{
  // Get publisher type from fully qualified name
  auto topic_name = _name;

  auto first = topic_name.find_first_of("@");
  // ERROR - no special part (requires at least 2 '@')
  if (first == std::string::npos)
  {
    return false;
  }
  // remove first '@'
  topic_name.erase(0, first + 1);
  // find where partition part ends
  first = topic_name.find_first_of("@");
  // ERROR - no special part (requires at least 2 '@')
  if (first == std::string::npos)
  {
    return false;
  }
  // find where type part ends
  auto last = topic_name.find_first_of("@", first + 1);
  // special case when there's no partition
  if (last == std::string::npos)
  {
    last = first;
    first = -1;
  }
  // get type
  _type = topic_name.substr(first + 1, last - first - 1);

  return (_type == "msg" || _type == "srv");
}
