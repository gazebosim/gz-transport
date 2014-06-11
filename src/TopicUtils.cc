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
bool TopicUtils::IsValidTopic(const std::string &_topic, const std::string &_ns)
{
	// The empty string is not valid.
	if (_topic == "")
		return false;

	// Only the character '~' of '/' is not valid.
	if (_topic == "~" || _topic == "/")
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
  	return false;

  return true;
}

//////////////////////////////////////////////////
std::string TopicUtils::GetScopedName(const std::string &_nameSpace,
	const std::string &_topic)
{
	// If the topic starts with '~' is considered a relative path and the
	// namespace will be prefixed.
	if (_topic.find("~/") == 0)
		return _nameSpace + _topic.substr(1);

	if (_topic.find("~") == 0)
		return _nameSpace + "/" + _topic.substr(1);


}
