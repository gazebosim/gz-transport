 /*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifndef _OPTITRACK_HH_
#define _OPTITRACK_HH_

#include <string>

class Optitrack
{
  /// \brief Creates an Optitrack object able to receive multicast updates
  /// from the Optitrack server containing the tracking information.
  /// \param[in] _localIP IP address of this machine.
  /// \param[in] _threshold Distance (m) to consider that the body has moved.
  public: Optitrack(const std::string &_localIP, float _threshold);

  /// \brief Default destructor.
  public: ~Optitrack() = default;

  /// \brief Receive OptiTrack updates.
  public: void Receive();

  /// \brief Unpack the data received from the network.
  /// \param[in] _data Buffer received.
  private: void Unpack(char *_data);

  /// \brief NatNet major version.
  public: const int NatNetVersionMajor = 2;

  /// \brief NatNet minor version.
  public: const int NatNetVersionMinor = 7;

  /// \brief Optitrack multicast address.
  private: const std::string MulticastAddress = "239.255.42.99";

  /// \brief Port used for sending/receiving tracking updates.
  private: const int PortData = 1511;

  /// \brief UDP socket used to received tracking updates.
  private: int dataSocket;

  /// \brief IP address associated to the multicast socket.
  private: std::string myIPAddress;

  /// \brief Distance (m.) to consider that the body has moved.
  private: float threshold;
};
#endif
