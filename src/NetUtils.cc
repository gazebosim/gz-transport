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

#include <cstdint>
#include <iostream>
#include <string>
#include "ignition/transport/NetUtils.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
bool NetUtils::ZmqToIp(const std::string &_addr, std::string &_ip)
{
  const std::string DelimiterBegin = "://";
  const std::string DelimiterEnd = ":";

  std::size_t found = _addr.find(DelimiterBegin);
  if (found == std::string::npos)
    return false;

  std::string ip = _addr.substr(found + DelimiterBegin.size(), _ip.size() - 1);

  found = ip.find(DelimiterEnd);
  if (found == std::string::npos)
    return false;

  _ip = ip.substr(0, found);

  // ToDo(caguero): Validate the ip address.

  return true;
}
