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

// \brief Used to read a DiscoveryInfo tuple.
#define Addr 0
#define Ctrl 1
#define Uuid 2

namespace ignition
{
  namespace transport
  {
    /// \class DiscoveryPrivate DiscoveryPrivate.hh
    /// \brief Private data for the Discovery class.
    class DiscoveryPrivate
    {
      /// \def Timestamp
      /// \brief Used to evaluate the validity of a discovery entry.
      typedef std::chrono::time_point<std::chrono::steady_clock> Timestamp;

      /// \brief Constructor.
      /// \param[in] _procUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode.
      public: DiscoveryPrivate(const uuid_t &_procUuid, bool _verbose);

      /// \brief Destructor.
      public: virtual ~DiscoveryPrivate();

      /// \brief Check the validity of the topic information. Each topic update
      /// has its own timestamp. This method iterates over the list of topics
      /// and invalid the old topics.
      public: void RunActivityService();

      /// \brief Broadcast periodic heartbeats.
      public: void RunHeartbitService();

      /// \brief Receive discovery messages.
      public: void RunReceptionService();

      /// \brief Each time the client calls Discover(), the discovery will try
      /// to discover the addressing information for the requested topic. This
      /// method will periodically retransmit the request to discover until
      /// the advertiser answers.
      public: void RunRetransmissionService();

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _msg Received message.
      /// \return 0 when success.
      public: int DispatchDiscoveryMsg(char *_msg);

      /// \brief Broadcast a discovery message.
      /// \param[in] _type Message type.
      /// \param[in] _topic Topic name.
      /// \param[in] _flags Optional flags.
      /// \return 0 when success.
      public: int SendMsg(uint8_t _type, const std::string &_topic,
        int _flags = 0);

      /// \brief Check if a topic has been advertised by me.
      /// \return true if the topic was advertised by me before.
      public: bool AdvertisedByMe(const std::string &_topic);

      /// \brief Print the current discovery state (info, activity, unknown).
      public: void PrintCurrentState();

      /// \brief Default activity interval value.
      /// \sa GetActivityInterval.
      /// \sa SetActivityInterval.
      public: static const unsigned int DefActivityInterval = 100;

      /// \brief Default hello interval value.
      /// \sa GetHelloInterval.
      /// \sa SetHelloInterval.
      public: static const unsigned int DefHeartbitInterval = 1000;

      /// \brief Default silence interval value.
      /// \sa GetMaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      public: static const unsigned int DefSilenceInterval = 3000;

      /// \brief Default retransmission interval value.
      /// \sa GetRetransmissionInterval.
      /// \sa SetRetransmissionInterval.
      public: static const unsigned int DefRetransmissionInterval = 1000;

      /// \brief Port used to broadcast the discovery messages.
      public: static const int DiscoveryPort = 11312;

      /// \brief Timeout used for receiving messages.
      public: static const int Timeout = 250;

      /// \brief Process UUID.
      public: uuid_t uuid;

      /// \brief UUID in string format.
      public: std::string uuidStr;

      /// \brief Silence interval value.
      /// \sa GetMaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      public: unsigned int silenceInterval;

      /// \brief Activity interval value.
      /// \sa GetActivityInterval.
      /// \sa SetActivityInterval.
      public: unsigned int activityInterval;

      /// \brief Retransmission interval value.
      /// \sa GetRetransmissionInterval.
      /// \sa SetRetransmissionInterval.
      public: unsigned int retransmissionInterval;

      /// \brief Heartbit interval value.
      /// \sa GetHeartbitInterval.
      /// \sa SetHeartbitInterval.
      public: unsigned int heartbitInterval;

      /// \brief Callback executed when new topics are discovered.
      public: DiscoveryCallback connectionCb;

      /// \brief Callback executed when new topics are invalid.
      public: DiscoveryCallback disconnectionCb;

      /// \brief Topics requested to discover but with no information yet.
      public: std::vector<std::string> unknownTopics;

      /// \brief Main topic information. For each topic we store a tuple that
      /// contains the 0MQ address, 0MQ control, and process UUID of the
      /// publisher.
      public: std::map<std::string, DiscoveryInfo> info;

      /// \brief Activity information. Every time there is a message from a
      /// remote node, its activity information is updated. If we do not hear
      /// from a node in a while, its entries in 'info' will be invalided.
      public: std::map<std::string, Timestamp> activity;

      /// \brief Print activity to stdout.
      public: bool verbose;

      /// \brief ZMQ context for the discovery beacon.
      public: zctx_t *ctx;

      /// \brief Discovery beacon.
      public: zbeacon_t *beacon;

      /// \brief Mutex to guarantee exclusive access between the threads.
      public: std::mutex mutex;

      /// \brief tTread in charge of receiving and handling incoming messages.
      public: std::thread *threadReception;

      /// \brief Thread in charge of sending HELLOs.
      public: std::thread *threadHeartbit;

      /// \brief Thread in charge of update the activity.
      public: std::thread *threadActivity;

      /// \brief Thread in charge of sending periodic SUB messages (if needed).
      public: std::thread *threadRetransmission;

      /// \brief Mutex to guarantee exclusive access to the exit variable.
      public: std::mutex exitMutex;

      /// \brief When true, the service threads will finish.
      public: bool exit;
    };
  }
}
#endif
