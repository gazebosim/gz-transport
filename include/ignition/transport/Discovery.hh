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

#include <uuid/uuid.h>
#include <functional>
#include <memory>
#include <string>
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Discovery Discovery.hh
    /// \brief A discovery class.
    class Discovery
    {
      /// \brief Constructor.
      /// \param[in] _procUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Discovery(const uuid_t &_procUuid, bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Discovery();

      /// \brief Advertise a new topic.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _addr ZeroMQ address of the topic's publisher.
      /// \param[in] _ctrl ZeroMQ control address of the topic's publisher.
      public: void Advertise(const std::string &_topic,
        const std::string &_addr, const std::string &_ctrl);

      /// \brief Request discovery information about a topic.
      /// \param[in] _topic Topic requested.
      public: void Discover(const std::string &_topic);

      /// \brief Unadvertise a topic. Broadcast a discovery message that will
      /// cancel all the discovery information for this topic.
      /// \param[in] _topic Topic to be unadvertised.
      /// \param[in] _addr 0MQ Address of the node unadvertising the topic.
      /// \param[in] _ctrl 0MQ Control addr of the node unadvertising the topic.
      public: void Unadvertise(const std::string &_topic,
        const std::string &_addr, const std::string &_ctrl);

      /// \brief Get the IP address of the host.
      /// \return A string with the host's IP address.
      public: std::string GetHostAddr();

      /// \brief The discovery checks the validity of the topic information
      /// every 'activity interval' time.
      /// \return The value in milliseconds.
      public: unsigned int GetActivityInterval();

      /// \brief Each node broadcasts periodic heartbeats to keep its topic
      /// information alive in the remote nodes. A HELLO message is sent after
      /// 'heartbit interval' time.
      /// \return The value in milliseconds.
      public: unsigned int GetHeartbitInterval();

      /// \brief After a discover request, the discovery will send periodic
      /// discovery requests every 'retransmission interval' time.
      /// \return The value in milliseconds.
      public: unsigned int GetRetransmissionInterval();

      /// \brief Get the maximum time allowed without receiving any discovery
      /// information from a node before canceling its entry.
      /// \return The value in milliseconds.
      public: unsigned int GetSilenceInterval();

      /// \brief Set the activity interval.
      /// \sa GetActivityInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetActivityInterval(unsigned int _ms);

      /// \brief Set the hello interval.
      /// \sa GetHeartbitInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetHeartbitInterval(unsigned int _ms);

      /// \brief Set the retransmission interval.
      /// \sa GetRetransmissionInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetRetransmissionInterval(unsigned int _ms);

      /// \brief Set the maximum silence interval.
      /// \sa GetSilenceInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetSilenceInterval(unsigned int _ms);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new node is connected, the callback will be execute. This
      /// version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetConnectionsCb(const DiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new node is connected, the callback will be execute. This
      /// version uses a member functions as callback.
      /// \param[in] _cb Function callback.
      public: template<class C> void SetConnectionsCb(
        void(C::*_cb)(const std::string &, const std::string &,
          const std::string &, const std::string &), C* _obj)
      {
        this->SetConnectionsCb(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));
      }

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a new node is disconnected, the callback will be execute.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetDisconnectionsCb(const transport::DiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a new node is disconnected, the callback will be execute.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback.
      public: template<class C> void SetDisconnectionsCb(
        void(C::*_cb)(const std::string &, const std::string &,
          const std::string &, const std::string &), C* _obj)
      {
        this->SetDisconnectionsCb(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3, std::placeholders::_4));
      }

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<DiscoveryPrivate> dataPtr;
    };
  }
}
#endif
