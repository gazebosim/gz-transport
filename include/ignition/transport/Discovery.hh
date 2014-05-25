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

#ifndef __IGN_TRANSPORT_DISCOVERY_HH_INCLUDED__
#define __IGN_TRANSPORT_DISCOVERY_HH_INCLUDED__

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
    /// \class Discovery Discovery.hh
    /// \brief A discovery class.
    class Discovery
    {
      public: static const int NewDiscoveryResponse = 0;
      public: static const int NewNodeDisconnected = 1;

      public: Discovery(const std::string &_addr,
        const std::string &_ctrl, const uuid_t &_procUuid,
        const uuid_t &_nodeUuid, bool _verbose = false);

      public: virtual ~Discovery();

      public: void Advertise(const std::string &_topic);

      public: void Discover(const std::string &_topic);

      public: void Unadvertise(const std::string &_topic);

      public: void SetMaxSilenceInterval(int _ms);

      public: void SetPollingInterval(int _ms);

      public: void SetSubInterval(int _ms);

      public: void RegisterDiscoverResp(const transport::DiscResponse &_cb);

      public: void RegisterDisconnectResp(const transport::DiscResponse &_cb);

      /// \brief Run one iteration of the discovery.
      private: void SpinOnce();

      /// \brief Receive discovery messages forever.
      private: void Spin();

      private: void UpdateActivity();

      private: void SendHello();

      private: void SendBye();

      private: void RetransmitSubscriptions();

      /// \brief Method in charge of receiving the discovery updates.
      private: void RecvDiscoveryUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      private: int DispatchDiscoveryMsg(char *_msg);

      /// \brief Send an ADVERTISE message to the discovery socket.
      /// \param[in] _type ADV or ADV_SVC.
      /// \param[in] _topic Topic to be advertised.
      /// \return 0 when success.
      private: int SendAdvertiseMsg(uint8_t _type, const std::string &_topic);

      /// \brief Send a SUBSCRIBE message to the discovery socket.
      /// \param[in] _type SUB or SUB_SVC.
      /// \param[in] _topic Topic name.
      /// \return 0 when success.
      private: int SendSubscribeMsg(uint8_t _type, const std::string &_topic);

      private: bool AdvertisedByMe(const std::string &_topic);

      private: static const int Timeout = 250;

      /// \brief Port used to broadcast the discovery messages.
      private: static const int DiscoveryPort = 11312;

      private: static const int DefSilenceInterval = 3000;
      private: static const int DefPollingInterval = 250;
      private: static const int DefSubInterval = 1000;
      private: static const int DefHelloInterval = 1000;

      private: std::string addr;
      private: std::string ctrlAddr;
      /// \brief Process UUID.
      private: uuid_t procUuid;
      private: uuid_t nodeUuid;

      /// \brief UDP broadcast port used for the transport.
      private: int bcastPort;

      private: int silenceInterval;
      private: int pollingInterval;
      private: int subInterval;
      private: int helloInterval;

      private: transport::DiscResponse newDiscoveryEvent;
      private: transport::DiscResponse newDisconnectionEvent;

      private: std::vector<std::string> advTopics;
      private: std::vector<std::string> unknownTopics;
      private: std::map<std::string, transport::DiscTopicInfo> info;
      private: std::map<std::string, transport::Timestamp> activity;

      /// \brief Print activity to stdout.
      private: int verbose;

      /// \brief ZMQ context for the discovery beacon.
      private: zctx_t *ctx;

      /// \brief Discovery beacon.
      private: zbeacon_t *beacon;

      /// \brief Mutex to guarantee exclusive access between the sockets.
      private: std::mutex mutex;

      /// \brief thread in charge of receiving and handling incoming messages.
      private: std::thread *threadInbound;

      /// \brief thread in charge of sending HELLOs.
      private: std::thread *threadHello;

      /// \brief thread in charge of update the activity.
      private: std::thread *threadActivity;

      /// \brief thread in charge of sending periodic SUB messages (if needed).
      private: std::thread *threadSub;

      /// \brief Mutex to guarantee exclusive access to exit variable.
      private: std::mutex exitMutex;

      /// \brief When true, the service thread will finish.
      private: bool exit;

      /// \brief Timeout used for receiving requests.
      public: int timeout;
    };
  }
}
#endif
