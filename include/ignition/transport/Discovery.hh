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

#ifdef _MSC_VER
# pragma warning(push, 0)
#endif
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    class DiscoveryPrivate;

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
      /// \brief NatNet major version.
      public: const int NatNetVersionMajor = 2;

      /// \brief NatNet minor version.
      public: const int NatNetVersionMinor = 7;

      #define MAX_NAMELENGTH              256

      /// \brief Constructor.
      /// \param[in] _pUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Discovery(const std::string &_pUuid, bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Discovery();

      public: void Unpack(char *pData);

      /// \brief Advertise a new message or service.
      /// \param[in] _advType Message (Msg) or service (Srv).
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _addr ZeroMQ address of the topic's publisher.
      /// \param[in] _ctrl ZeroMQ control address of the topic's publisher.
      /// \param[in] _nUuid Node UUID.
      /// \param[in] _scope Topic scope.
      public: void Advertise(const MsgType &_advType,
                             const std::string &_topic,
                             const std::string &_addr,
                             const std::string &_ctrl,
                             const std::string &_nUuid,
                             const Scope &_scope);

      /// \brief Request discovery information about a topic or service.
      /// When using this method with messages, the user might want to use
      /// SetConnectionsCb() and SetDisconnectionCb(), that register callbacks
      /// that will be executed when the topic address is discovered or when the
      /// node providing the topic is disconnected.
      /// When using this method with services, the user might want to use
      /// SetConnectionsSrvCb() and SetDisconnectionSrvCb(), that register
      /// callbacks that will be executed when the service address is discovered
      /// or when the node providing the service is disconnected.
      /// \sa SetConnectionsCb.
      /// \sa SetDisconnectionsCb.
      /// \sa SetConnectionsSrvCb.
      /// \sa SetDisconnectionsSrvCb.
      /// \param[in] _topic Topic name requested.
      /// \param[in] _isSrv True if the topic corresponds to a service.
      public: void Discover(const std::string &_topic, bool _isSrv);

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

      /// \brief Unadvertise a new message or service. Broadcast a discovery
      /// message that will cancel all the discovery information for the topic
      /// or service advertised by a specific node.
      /// \param[in] _unadvType Message (Msg) or service (Srv).
      /// \param[in] _topic Topic/service name to be unadvertised.
      /// \param[in] _nUuid Node UUID.
      public: void Unadvertise(const MsgType &_unadvType,
                               const std::string &_topic,
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

      /// \brief Check the validity of the topic information. Each topic update
      /// has its own timestamp. This method iterates over the list of topics
      /// and invalids the old topics.
      public: void RunActivityTask();

      /// \brief Broadcast periodic heartbeats.
      public: void RunHeartbeatTask();

      /// \brief Receive discovery messages.
      public: void RunReceptionTask();

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _fromIp IP address of the message sender.
      /// \param[in] _msg Received message.
      public: void DispatchDiscoveryMsg(const std::string &_fromIp,
                                        char *_msg);

      /// \brief Broadcast a discovery message.
      /// \param[in] _type Message type.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ Address.
      /// \param[in] _ctrl 0MQ control address.
      /// \param[in] _nUuid Node's UUID.
      /// \param[in] _flags Optional flags. Currently, the flags are not used
      /// but they will in the future for specifying things like compression,
      /// or encryption.
      public: void SendMsg(uint8_t _type,
                           const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_nUuid,
                           const Scope &_scope,
                           int _flags = 0);

      /// \brief Print the current discovery state (info, activity, unknown).
      public: void PrintCurrentState();

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void GetTopicList(std::vector<std::string> &_topics) const;

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void GetServiceList(std::vector<std::string> &_services) const;

      /// \brief Get mutex used in the Discovery class.
      /// \return The discovery mutex.
      public: std::recursive_mutex& GetMutex();

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<DiscoveryPrivate> dataPtr;
    };
  }
}
#endif
