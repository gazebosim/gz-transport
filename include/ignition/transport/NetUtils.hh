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

#ifndef __IGN_TRANSPORT_NETUTILS_HH_INCLUDED__
#define __IGN_TRANSPORT_NETUTILS_HH_INCLUDED__

#include <cstdint>
#include <string>

namespace ignition
{
  namespace transport
  {
      /// \class NetUtils NetUtils.hh
      /// \brief Network utilities.
      class NetUtils
      {
      /// \brief Cast an IP address into an int.
      /// \param[in] _ip IP address in string format.
      /// \return an int where each byte contains a field of the IP address.
      /*public: static uint32_t IpToInt(const std::string &_ip);

      /// \brief Cast an int into a string IP address.
      /// \param[in] _ip IP address in int format.
      /// \return a string containing the IP address.
      public: static std::string IntToIp(uint32_t _ip);

      /// \brief Return if an IP address belongs to a given <subnet, netmask>.
      /// \param[in] _ip IP address.
      /// \param[in] _network Network address.
      /// \param[in] _mask Network mask
      /// \return True if _ip is a valid IP address in the subnet '_network'
      /// applying the netmask '_mask'.
      public: static bool IsIpInRange(const std::string &_ip,
                                      const std::string &_network,
                                      const std::string &_mask);

      /// \brief Return the network mask of the main network interface.
      /// \return The netmask. Ex: "255.255.255.0"
      /// ToDo(caguero) Make it portable.
      public: static std::string GetNetmask();*/

      /// \brief Return the ip address of a 0MQ endpoint. Ex: 'tcp://10.0.0.1'
      /// \param[in] _addr 0MQ address.
      /// \param[out] _ip Ip address.
      /// \return True if the conversion was done.
      public: static bool ZmqToIp(const std::string &_addr, std::string &_ip);
    };
  }
}

#endif
