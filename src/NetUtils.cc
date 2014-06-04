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
/*uint32_t NetUtils::IpToInt(const std::string &_ip)
{
  int a, b, c, d;
  uint32_t addr = 0;

  // const std::regex ipPattern("(\\d{1,3}):(\\d{1,3}):(\\d{1,3}):(\\d{1,3})");

  // Sequence of sub-matches.
  // std::match_results<std::string::const_iterator> result;

  // Match the IP address with the regular expression.
  // bool valid = std::regex_match(_ip, result, ipPattern);

  // if (!valid)
  // {
  //   std::cerr << "NetUtils::IpToInt() error: [" << _ip << "] is not a valid "
  //             << "IP address." << std::endl;
  //   return 0;
  // }

  if (sscanf(_ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    return 0;

  addr  = a << 24;
  addr |= b << 16;
  addr |= c << 8;
  addr |= d;
  return addr;
}

//////////////////////////////////////////////////
std::string NetUtils::IntToIp(uint32_t _ip)
{
  auto a = (_ip >> 24) & 0xFF;
  auto b = (_ip >> 16) & 0xFF;
  auto c = (_ip >> 8) & 0xFF;
  auto d = _ip & 0xFF;

  return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) +
         "." + std::to_string(d);
}

//////////////////////////////////////////////////
bool NetUtils::IsIpInRange(const std::string &_ip,
                           const std::string &_network,
                           const std::string &_mask)
{
  auto ip_addr = IpToInt(_ip);
  auto network_addr = IpToInt(_network);
  auto mask_addr = IpToInt(_mask);

  auto net_lower = (network_addr & mask_addr);
  auto net_upper = (net_lower | (~mask_addr));

  return ip_addr >= net_lower && ip_addr <= net_upper;
}

//////////////////////////////////////////////////
std::string NetUtils::GetNetmask()
{
  return std::string("255.255.255.0");
}*/

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
