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

#ifndef __IGN_TRANSPORT_DISCOVERY_public_HH_INCLUDED__
#define __IGN_TRANSPORT_DISCOVERY_public_HH_INCLUDED__

#include <czmq.h>
#include <uuid/uuid.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class DiscoveryPrivate DiscoveryPrivate.hh
    /// \brief Private data for the Discovery class.
    class DiscoveryPrivate
    {
      public: DiscoveryPrivate(const uuid_t &_procUuid, bool _verbose);

      public: virtual ~DiscoveryPrivate();

      /// \brief Run one iteration of the discovery.
      public: void SpinOnce();

      /// \brief Receive discovery messages forever.
      public: void Spin();

      public: void UpdateActivity();

      public: void SendHello();

      public: void SendBye();

      public: void RetransmitSubscriptions();

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      public: int DispatchDiscoveryMsg(char *_msg);

      /// \brief Send an ADVERTISE message to the discovery socket.
      /// \param[in] _type ADV or ADV_SVC.
      /// \param[in] _topic Topic to be advertised.
      /// \return 0 when success.
      public: int SendAdvertiseMsg(uint8_t _type, const std::string &_topic);

      /// \brief Send a SUBSCRIBE message to the discovery socket.
      /// \param[in] _type SUB or SUB_SVC.
      /// \param[in] _topic Topic name.
      /// \return 0 when success.
      public: int SendSubscribeMsg(uint8_t _type, const std::string &_topic);

      public: bool AdvertisedByMe(const std::string &_topic);

      /// \brief Timeout used for receiving requests.
      public: static const int Timeout = 250;

      /// \brief Port used to broadcast the discovery messages.
      public: static const int DiscoveryPort = 11312;

      public: static const int DefSilenceInterval = 3000;
      public: static const int DefPollingInterval = 250;
      public: static const int DefSubInterval = 1000;
      public: static const int DefHelloInterval = 1000;

      /// \brief Process UUID.
      public: uuid_t procUuid;

      public: unsigned int silenceInterval;
      public: unsigned int pollingInterval;
      public: unsigned int subInterval;
      public: unsigned int helloInterval;

      public: transport::DiscResponse newDiscoveryEvent;
      public: transport::DiscResponse newDisconnectionEvent;

      public: std::vector<std::string> unknownTopics;
      public: std::map<std::string, transport::DiscTopicInfo> info;
      public: std::map<std::string, transport::Timestamp> activity;

      /// \brief Print activity to stdout.
      public: int verbose;

      /// \brief ZMQ context for the discovery beacon.
      public: zctx_t *ctx;

      /// \brief Discovery beacon.
      public: zbeacon_t *beacon;

      /// \brief Mutex to guarantee exclusive access between the sockets.
      public: std::mutex mutex;

      /// \brief thread in charge of receiving and handling incoming messages.
      public: std::thread *threadInbound;

      /// \brief thread in charge of sending HELLOs.
      public: std::thread *threadHello;

      /// \brief thread in charge of update the activity.
      public: std::thread *threadActivity;

      /// \brief thread in charge of sending periodic SUB messages (if needed).
      public: std::thread *threadSub;

      /// \brief Mutex to guarantee exclusive access to exit variable.
      public: std::mutex exitMutex;

      /// \brief When true, the service thread will finish.
      public: bool exit;
    };
  }
}
#endif
