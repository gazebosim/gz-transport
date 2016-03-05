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

#include <string>
#include <vector>

#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    /// \brief Determine if an IP is private.
    /// Reference: https://github.com/ros/ros_comm/blob/hydro-devel/clients/
    /// roscpp/src/libros/network.cpp
    /// \param[in] _ip Input IP address.
    /// \return true if the IP address is private.
    bool isPrivateIP(const char *_ip);

    /// \brief Determine if an IP is private.
    /// \param[in] _hostname Hostname
    /// \param[out] _ip IP associated to the input hostname.
    /// \return 0 when success.
    int hostnameToIp(char *_hostname, std::string &_ip);

    /// \brief Determine IP or hostname.
    /// Reference: https://github.com/ros/ros_comm/blob/hydro-devel/clients/
    /// roscpp/src/libros/network.cpp
    /// \return The IP or hostname of this host.
    IGNITION_VISIBLE
    std::string determineHost();

    /// \brief Determine the list of network interfaces for this machine.
    /// Reference: https://github.com/ros/ros_comm/blob/hydro-devel/clients/
    /// roscpp/src/libros/network.cpp
    /// \return The list of network interfaces.
    IGNITION_VISIBLE
    std::vector<std::string> determineInterfaces();

    /// \brief Determine the computer's hostname.
    /// \return The computer's hostname.
    IGNITION_VISIBLE
    std::string hostname();

    /// \brief Determine your login name.
    /// \return Name used to gain access to the computer.
    IGNITION_VISIBLE
    std::string username();
  }
}

#endif
