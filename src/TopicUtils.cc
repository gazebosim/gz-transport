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

#include <iostream>

#include <algorithm>
#include <regex>
#include <string>

#include "gz/transport/TopicUtils.hh"

namespace gz::transport
{

/// \brief The separator used within the liveliness token.
const std::string TopicUtils::kTokenSeparator = "/";

/// \brief A common prefix for all liveliness tokens.
const std::string TopicUtils::kTokenPrefix = "@gz";

const char TopicUtils::kSlashReplacement = '%';

//////////////////////////////////////////////////
bool TopicUtils::IsValidNamespace(const std::string &_ns)
{
  // An empty namespace is valid, so take a shortcut here.
  if (_ns.empty())
    return true;

  // Too long string is not valid.
  if (_ns.size() > kMaxNameLength)
    return false;

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

  // If the topic name has a ':=' is not valid.
  if (_ns.find(":=") != std::string::npos)
    return false;

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::IsValidPartition(const std::string &_partition)
{
  // A valid namespace is also a valid partition.
  return IsValidNamespace(_partition);
}

//////////////////////////////////////////////////
bool TopicUtils::IsValidTopic(const std::string &_topic)
{
  return IsValidNamespace(_topic) && !_topic.empty();
}

//////////////////////////////////////////////////
bool TopicUtils::FullyQualifiedName(const std::string &_partition,
  const std::string &_ns, const std::string &_topic, std::string &_name)
{
  // Sanity check, first things first.
  if (!IsValidPartition(_partition) || !IsValidNamespace(_ns) ||
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

  // Too long string is not valid.
  if (_name.size() > kMaxNameLength)
    return false;

  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::DecomposeFullyQualifiedTopic(
    const std::string &_fullyQualifiedName,
    std::string &_partition,
    std::string &_namespaceAndTopic)
{
  const std::size_t firstAt = _fullyQualifiedName.find_first_of("@");
  const std::size_t lastAt = _fullyQualifiedName.find_last_of("@");

  if ( firstAt != 0
    || firstAt == lastAt
    || lastAt == _fullyQualifiedName.size() - 1)
  {
    return false;
  }

  std::string possiblePartition = _fullyQualifiedName.substr(
    firstAt + 1, lastAt - firstAt - 1);
  std::string possibleTopic = _fullyQualifiedName.substr(lastAt + 1);

  if (!IsValidPartition(possiblePartition) || !IsValidTopic(possibleTopic))
  {
    return false;
  }

  _partition = possiblePartition;
  _namespaceAndTopic = possibleTopic;
  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::DecomposeLivelinessToken(
    const std::string &_token,
    std::string &_prefix,
    std::string &_partition,
    std::string &_namespaceAndTopic,
    std::string &_pUUID,
    std::string &_nUUID,
    std::string &_entityType,
    std::string &_msgType)
{
  std::cerr << "0" << std::endl;
  std::cerr << _token << std::endl;
  auto nDelims = static_cast<int>(
    std::count(_token.begin(), _token.end(), '/'));
  if (nDelims != 6)
    return false;

  std::cerr << "1" << std::endl;

  std::string token = _token;

  std::size_t firstAt = token.find_first_of("@");
  if ( firstAt != 0)
    return false;

  std::cerr << "2" << std::endl;

  firstAt = token.find_first_of("/");

  std::string possiblePrefix = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  std::cerr << "3" << std::endl;

  // Partition
  std::string possiblePartition = token.substr(0, firstAt);
  possiblePartition = Demangle(possiblePartition);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  std::cerr << "4" << std::endl;

  // Topic
  std::string possibleTopic = token.substr(0, firstAt);
  possibleTopic = Demangle(possibleTopic);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  std::cerr << "5" << std::endl;

  // Process UUID
  std::string possibleProcUUID = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  std::cerr << "6" << std::endl;

  // Node UUID
  std::string possibleNodeUUID = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  std::cerr << "7" << std::endl;

  // Entity type
  std::string possibleEntityType = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  // MsgType
  std::string possibleMsgType = token;

  std::cerr << possiblePartition << std::endl;
  std::cerr << possibleTopic << std::endl;

  if (!DecomposeFullyQualifiedTopic(possiblePartition + possibleTopic,
    possiblePartition, possibleTopic))
  {
    std::cerr << "error 1" << std::endl;
    return false;
  }

  std::cerr << possiblePartition << std::endl;
  std::cerr << possibleTopic << std::endl;

  if (!IsValidPartition(possiblePartition) || !IsValidTopic(possibleTopic))
  {
    return false;
  }

  std::cerr << "8" << std::endl;

  _prefix = possiblePrefix;
  _partition = possiblePartition;
  _namespaceAndTopic = possibleTopic;
  _pUUID = possibleProcUUID;
  _nUUID = possibleNodeUUID;
  _entityType = possibleEntityType;
  _msgType = possibleMsgType;
  return true;
}

//////////////////////////////////////////////////
bool TopicUtils::DecomposeLivelinessToken(
    const std::string &_token,
    std::string &_prefix,
    std::string &_partition,
    std::string &_namespaceAndTopic,
    std::string &_pUUID,
    std::string &_nUUID,
    std::string &_entityType,
    std::string &_reqType,
    std::string &_repType)
{
  auto nDelims = static_cast<int>(
    std::count(_token.begin(), _token.end(), '/'));
  if (nDelims != 7)
    return false;

  std::string token = _token;

  std::size_t firstAt = token.find_first_of("@");
  if ( firstAt != 0)
    return false;

  firstAt = token.find_first_of("/", 1);

  std::string possiblePrefix = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  // Partition
  std::string possiblePartition = token.substr(0, firstAt);
  possiblePartition = Demangle(possiblePartition);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  // Topic
  std::string possibleTopic = token.substr(0, firstAt);
  possibleTopic = Demangle(possibleTopic);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  // Process UUID
  std::string possibleProcUUID = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  // Node UUID
  std::string possibleNodeUUID = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  // Entity type
  std::string possibleEntityType = token.substr(0, firstAt);
  token.erase(0, firstAt + 1);

  firstAt = token.find_first_of("/");
  if ( firstAt == 0)
    return false;

  // ReqType
  std::string possibleReqType = token.substr(0, firstAt);;
  token.erase(0, firstAt + 1);

  // RepType
  std::string possibleRepType = token;

  if (!DecomposeFullyQualifiedTopic(possiblePartition + possibleTopic,
    possiblePartition, possibleTopic))
  {
    return false;
  }

  if (!IsValidPartition(possiblePartition) || !IsValidTopic(possibleTopic))
  {
    return false;
  }

  _prefix = possiblePrefix;
  _partition = possiblePartition;
  _namespaceAndTopic = possibleTopic;
  _pUUID = possibleProcUUID;
  _nUUID = possibleNodeUUID;
  _entityType = possibleEntityType;
  _reqType = possibleReqType;
  _repType = possibleRepType;
  return true;
}

//////////////////////////////////////////////////
std::string TopicUtils::AsValidTopic(const std::string &_topic)
{
  std::string validTopic{_topic};

  // Substitute spaces with _
  validTopic = std::regex_replace(validTopic, std::regex(" "), "_");

  // Remove special characters and combinations
  validTopic = std::regex_replace(validTopic, std::regex("@|~|//|:="), "");

  if (!IsValidTopic(validTopic))
  {
    return std::string();
  }

  return validTopic;
}

//////////////////////////////////////////////////
std::string TopicUtils::CreateLivelinessToken(
  const std::string &_fullyQualifiedTopic,
  const std::string &_pUuid,
  const std::string &_nUuid,
  const std::string &_entityType,
  const std::string &_msgTypeName)
{
  std::string partition;
  std::string topic;
  if (!DecomposeFullyQualifiedTopic(_fullyQualifiedTopic, partition, topic))
    return "";

  std::cerr << partition << std::endl;
  std::cerr << topic << std::endl;

  return
    kTokenPrefix + kTokenSeparator +
    Mangle(partition) + kTokenSeparator +
    Mangle(topic) + kTokenSeparator +
    _pUuid + kTokenSeparator +
    _nUuid + kTokenSeparator +
    _entityType + kTokenSeparator +
    _msgTypeName;
}

//////////////////////////////////////////////////
std::string TopicUtils::CreateLivelinessToken(
  const std::string &_fullyQualifiedTopic,
  const std::string &_pUuid,
  const std::string &_nUuid,
  const std::string &_entityType,
  const std::string &_reqTypeName,
  const std::string &_repTypeName)
{
  std::string partition;
  std::string topic;
  if (!DecomposeFullyQualifiedTopic(_fullyQualifiedTopic, partition, topic))
    return "";

  std::cerr << partition << std::endl;
  std::cerr << topic << std::endl;

  return
    kTokenPrefix + kTokenSeparator +
    Mangle(partition) + kTokenSeparator +
    Mangle(topic) + kTokenSeparator +
    _pUuid + kTokenSeparator +
    _nUuid + kTokenSeparator +
    _entityType + kTokenSeparator +
    _reqTypeName + kTokenSeparator +
    _repTypeName;
}

//////////////////////////////////////////////////
std::string TopicUtils::Mangle(const std::string &_input)
{
  std::string output = "";
  for (std::size_t i = 0; i < _input.length(); ++i)
  {
    if (_input[i] == '/')
      output += kSlashReplacement;
    else if (_input[i] == '@')
      output += '_';
    else
      output += _input[i];
  }
  return output;
}

//////////////////////////////////////////////////
std::string TopicUtils::Demangle(const std::string &_input)
{
  std::string output = "";
  for (std::size_t i = 0; i < _input.length(); ++i)
  {
    if (_input[i] == kSlashReplacement)
      output += '/';
    else if (_input[i] == '_')
      output += '@';
    else
      output += _input[i];
  }
  return output;
}

}  // namespace gz::transport
