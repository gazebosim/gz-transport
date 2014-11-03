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

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

# pragma warning(push, 0)
#include <functional>
#include <memory>
#include <string>
# pragma warning(pop)

#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Discovery Discovery.hh ignition/transport/Discovery.hh
    /// \brief A discovery class that implements a distributed topic discovery
    /// protocol. It uses UDP broadcast for sending/receiving messages and
    /// stores updated topic information. The discovery clients can request
    /// the discovery of a topic or the advertisement of a local topic. The
    /// discovery uses heartbeats to track the state of other peers in the
    /// network. The discovery clients can register callbacks to detect when
    /// new topics are discovered or topics are no longer available.
    class IGNITION_VISIBLE Discovery
    {
      /// \brief Constructor.
      /// \param[in] _pUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Discovery(const std::string &_pUuid, bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Discovery() = default;

      /// \brief Advertise a new message.
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _addr ZeroMQ address of the topic's publisher.
      /// \param[in] _ctrl ZeroMQ control address of the topic's publisher.
      /// \param[in] _nUuid Node UUID.
      /// \param[in] _scope Topic scope.
      public: void AdvertiseMsg(const std::string &_topic,
                                const std::string &_addr,
                                const std::string &_ctrl,
                                const std::string &_nUuid,
                                const Scope &_scope = Scope::All);

      /// \brief Advertise a new service.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _addr ZeroMQ address of the service provider.
      /// \param[in] _id ZeroMQ identity for the socket receiving the response.
      /// \param[in] _nUuid Node UUID.
      /// \param[in] _scope Topic scope.
      public: void AdvertiseSrv(const std::string &_topic,
                                const std::string &_addr,
                                const std::string &_id,
                                const std::string &_nUuid,
                                const Scope &_scope = Scope::All);

      /// \brief Request discovery information about a topic. The user
      /// might want to use this function with SetConnectionsCb() and
      /// SetDisconnectionCb(), that register callbacks that will be executed
      /// when the topic address is discovered or when the node providing the
      /// topic is disconnected.
      /// \sa SetConnectionsCb.
      /// \sa SetDisconnectionsCb.
      /// \param[in] _topic Topic requested.
      public: void DiscoverMsg(const std::string &_topic);

      /// \brief Request discovery information about a service. The user
      /// might want to use this function with SetConnectionsSrvCb() and
      /// SetDisconnectionSrvCb(), that register callbacks that will be executed
      /// when the service address is discovered or when the node providing the
      /// service is disconnected.
      /// \sa SetConnectionsSrvCb.
      /// \sa SetDisconnectionsSrvCb.
      /// \param[in] _topic Topic requested.
      public: void DiscoverSrv(const std::string &_topic);

      /// \brief Get all the addresses known for a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _addresses Addresses requested.
      /// \return True if the topic is found and there is at least one address.
      public: bool GetMsgAddresses(const std::string &_topic,
                                   Addresses_M &_addresses);

      /// \brief Get all the addresses known for a given service.
      /// \param[in] _topic Service name.
      /// \param[out] _addresses Addresses requested.
      /// \return True if the topic is found and there is at least one address.
      public: bool GetSrvAddresses(const std::string &_topic,
                                   Addresses_M &_addresses);

      /// \brief Unadvertise a topic. Broadcast a discovery message that will
      /// cancel all the discovery information for the topic advertised by a
      /// specific node.
      /// \param[in] _topic Topic to be unadvertised.
      /// \param[in] _nUuid Node UUID of the publisher.
      public: void UnadvertiseMsg(const std::string &_topic,
                                  const std::string &_nUuid);

      /// \brief Unadvertise a service. Broadcast a discovery message that
      /// will cancel all the discovery information for the service advertised
      /// by a specific node.
      /// \param[in] _topic Topic to be unadvertised.
      /// \param[in] _nUuid Node UUID of the publisher.
      public: void UnadvertiseSrv(const std::string &_topic,
                                  const std::string &_nUuid);

      /// \brief Get the IP address of this host.
      /// \return A string with this host's IP address.
      public: std::string GetHostAddr() const;

      /// \brief The discovery checks the validity of the topic information
      /// every 'activity interval' milliseconds.
      /// \sa SetActivityInterval.
      /// \return The value in milliseconds.
      public: unsigned int GetActivityInterval() const;

      /// \brief Each node broadcasts periodic heartbeats to keep its topic
      /// information alive in other nodes. A heartbeat message is sent after
      /// 'heartbeat interval' milliseconds.
      /// \sa SetHeartbeatInterval.
      /// \return The value in milliseconds.
      public: unsigned int GetHeartbeatInterval() const;

      /// \brief While a topic is being advertised by a node, a beacon is sent
      /// periodically every 'advertise interval' milliseconds.
      /// \sa SetAdvertiseInterval.
      /// \return The value in milliseconds.
      public: unsigned int GetAdvertiseInterval() const;

      /// \brief Get the maximum time allowed without receiving any discovery
      /// information from a node before canceling its entries.
      /// \sa SetSilenceInterval.
      /// \return The value in milliseconds.
      public: unsigned int GetSilenceInterval() const;

      /// \brief Set the activity interval.
      /// \sa GetActivityInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetActivityInterval(const unsigned int _ms);

      /// \brief Set the heartbeat interval.
      /// \sa GetHeartbeatInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetHeartbeatInterval(const unsigned int _ms);

      /// \brief Set the advertise interval.
      /// \sa GetAdvertiseInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetAdvertiseInterval(const unsigned int _ms);

      /// \brief Set the maximum silence interval.
      /// \sa GetSilenceInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetSilenceInterval(const unsigned int _ms);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new topic is connected, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetConnectionsCb(const DiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new topic is discovered, the callback will be executed.
      /// This version uses a member functions as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _topic Topic name
      ///                _addr ZeroMQ address of the publisher.
      ///                _ctrl ZeroMQ control address of the publisher
      ///                _pUuid UUID of the process publishing the topic.
      ///                _nUuid UUID of the node publishing the topic.
      ///                _scope Topic scope.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetConnectionsCb(
        void(C::*_cb)(const std::string &_topic, const std::string &_addr,
          const std::string &_ctrl, const std::string &_pUuid,
          const std::string &_nUuid, const Scope &_scope),
        C *_obj)
      {
        this->SetConnectionsCb(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
            std::placeholders::_6));
      }

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetDisconnectionsCb(const transport::DiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _topic Topic name
      ///                _addr ZeroMQ address of the publisher.
      ///                _ctrl ZeroMQ control address of the publisher
      ///                _pUuid UUID of the process publishing the topic.
      ///                _nUuid UUID of the node publishing the topic.
      ///                _scope Topic scope.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetDisconnectionsCb(
        void(C::*_cb)(const std::string &_topic, const std::string &_addr,
          const std::string &_ctrl, const std::string &_pUuid,
          const std::string &_nUuid, const Scope &_scope),
        C *_obj)
      {
        this->SetDisconnectionsCb(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
            std::placeholders::_6));
      }

      /// \brief Register a callback to receive discovery connection events for
      /// services.
      /// Each time a new service is available, the callback will be
      /// executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetConnectionsSrvCb(const DiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events for
      /// services.
      /// Each time a new service is available, the callback will be executed.
      /// This version uses a member functions as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _topic Topic name
      ///                _addr ZeroMQ address of the publisher.
      ///                _ctrl ZeroMQ control address of the publisher
      ///                _pUuid UUID of the process publishing the topic.
      ///                _nUuid UUID of the node publishing the topic.
      ///                _scope Topic scope.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetConnectionsSrvCb(
        void(C::*_cb)(const std::string &_topic, const std::string &_addr,
          const std::string &_ctrl, const std::string &_pUuid,
          const std::string &_nUuid, const Scope &_scope),
        C *_obj)
      {
        this->SetConnectionsSrvCb(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
            std::placeholders::_6));
      }

      /// \brief Register a callback to receive discovery disconnection events
      /// for services.
      /// Each time a service is no longer available, the callback will be
      /// executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetDisconnectionsSrvCb(
        const transport::DiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a service is no longer available, the callback will be
      /// executed.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _topic Topic name
      ///                _addr ZeroMQ address of the publisher.
      ///                _ctrl ZeroMQ control address of the publisher
      ///                _pUuid UUID of the process publishing the topic.
      ///                _nUuid UUID of the node publishing the topic.
      ///                _scope Topic scope.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetDisconnectionsSrvCb(
        void(C::*_cb)(const std::string &_topic, const std::string &_addr,
          const std::string &_ctrl, const std::string &_pUuid,
          const std::string &_nUuid, const Scope &_scope),
        C *_obj)
      {
        this->SetDisconnectionsSrvCb(
          std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
            std::placeholders::_6));
      }

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<DiscoveryPrivate> dataPtr;
    };
  }
}
#endif
