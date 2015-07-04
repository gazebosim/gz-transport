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

#ifndef __IGN_TRANSPORT_DISCOVERY_PRIVATE_HH_INCLUDED__
#define __IGN_TRANSPORT_DISCOVERY_PRIVATE_HH_INCLUDED__

#ifdef _MSC_VER
# pragma warning(push, 0)
#endif
#ifdef _WIN32
  #include <Winsock2.h>
#else
  #include <arpa/inet.h>
#endif
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class DiscoveryPrivate DiscoveryPrivate.hh
    /// ignition/transport/DiscoveryPrivate.hh
    /// \brief Private data for the Discovery class.
    class IGNITION_VISIBLE DiscoveryPrivate
    {
      /// \brief Constructor.
      public: DiscoveryPrivate() = default;

      /// \brief Destructor.
      public: virtual ~DiscoveryPrivate() = default;

      /// \brief Default activity interval value (ms.).
      /// \sa GetActivityInterval.
      /// \sa SetActivityInterval.
      public: static const unsigned int DefActivityInterval = 100;

      /// \brief Default heartbeat interval value (ms.).
      /// \sa GetHeartbeatInterval.
      /// \sa SetHeartbeatInterval.
      public: static const unsigned int DefHeartbeatInterval = 1000;

      /// \brief Default silence interval value (ms.).
      /// \sa GetMaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      public: static const unsigned int DefSilenceInterval = 3000;

      /// \brief Default advertise interval value (ms.).
      /// \sa GetAdvertiseInterval.
      /// \sa SetAdvertiseInterval.
      public: static const unsigned int DefAdvertiseInterval = 1000;

      /// \brief Port used to broadcast the discovery messages.
      public: static const int DiscoveryPort = 11319;

      /// \brief IP Address used for multicast.
      public: const std::string MulticastGroup = "224.0.0.7";

      /// \brief Timeout used for receiving messages (ms.).
      public: static const int Timeout = 250;

      /// \brief Longest string to receive.
      public: static const int MaxRcvStr = 65536;

      /// \brief Discovery protocol version.
      static const uint8_t Version = 2;

      /// \brief Host IP address.
      public: std::string hostAddr;

      /// \brief List of host network interfaces.
      public: std::vector<std::string> hostInterfaces;

      /// \brief Process UUID.
      public: std::string pUuid;

      /// \brief Silence interval value (ms.).
      /// \sa GetMaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      public: unsigned int silenceInterval;

      /// \brief Activity interval value (ms.).
      /// \sa GetActivityInterval.
      /// \sa SetActivityInterval.
      public: unsigned int activityInterval;

      /// \brief Advertise interval value (ms.).
      /// \sa GetAdvertiseInterval.
      /// \sa SetAdvertiseInterval.
      public: unsigned int advertiseInterval;

      /// \brief Heartbeat interval value (ms.).
      /// \sa GetHeartbeatInterval.
      /// \sa SetHeartbeatInterval.
      public: unsigned int heartbeatInterval;

      /// \brief Callback executed when new topics are discovered.
      public: DiscoveryCallback connectionCb;

      /// \brief Callback executed when new topics are invalid.
      public: DiscoveryCallback disconnectionCb;

      /// \brief Callback executed when new services are discovered.
      public: DiscoveryCallback connectionSrvCb;

      /// \brief Callback executed when a service is no longer available.
      public: DiscoveryCallback disconnectionSrvCb;

      /// \brief Message addressing information.
      public: TopicStorage infoTopics;

      /// \brief Activity information. Every time there is a message from a
      /// remote node, its activity information is updated. If we do not hear
      /// from a node in a while, its entries in 'info' will be invalided. The
      /// key is the process uuid.
      public: std::map<std::string, Timestamp> activity;

      /// \brief Print discovery information to stdout.
      public: bool verbose;

      /// \brief UDP socket used for sending/receiving discovery messages.
      public: std::vector<int> sockets;

      /// \brief Internet socket address for sending to the multicast group.
      public: sockaddr_in mcastAddr;

      /// \brief Mutex to guarantee exclusive access between the threads.
      public: std::recursive_mutex mutex;

      /// \brief Thread in charge of receiving and handling incoming messages.
      public: std::thread threadReception;

      /// \brief Thread in charge of sending heartbeats.
      public: std::thread threadHeartbeat;

      /// \brief Thread in charge of update the activity.
      public: std::thread threadActivity;

      /// \brief Mutex to guarantee exclusive access to the exit variable.
      public: std::recursive_mutex exitMutex;

      /// \brief When true, the service threads will finish.
      public: bool exit;

#ifdef _WIN32
      /// \brief True when the reception thread is finishing.
      public: bool threadReceptionExiting = true;
      /// \brief True when the hearbeat thread is finishing.
      public: bool threadHeartbeatExiting = true;
      /// \brief True when the activity thread is finishing.
      public: bool threadActivityExiting = true;
#endif

      /// \brief When true, the service is enabled.
      public: bool enabled;
    };
  }
}
#endif
