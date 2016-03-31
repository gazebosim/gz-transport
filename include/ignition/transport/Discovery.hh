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

#ifdef _WIN32
  // For socket(), connect(), send(), and recv().
  #include <Winsock2.h>
  // Type used for raw data on this platform.
  using raw_type = char;
#else
  // For sockaddr_in
  #include <netinet/in.h>
  // Type used for raw data on this platform
  using raw_type = void;
#endif

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    class DiscoveryPrivate;
    class Publisher;

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
      public: Discovery(const std::string &_pUuid,
                        const bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Discovery();

      /// \brief Start the discovery service. You probably want to register the
      /// callbacks for receiving discovery notifications before starting the
      /// service.
      public: void Start();

      /// \brief Advertise a new message.
      /// \param[in] _publisher Publisher's information to advertise.
      /// \return True if the method succeed or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool AdvertiseMsg(const MessagePublisher &_publisher);

      /// \brief Advertise a new service.
      /// \param[in] _publisher Publisher's information to advertise.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool AdvertiseSrv(const ServicePublisher &_publisher);

      /// \brief Request discovery information about a topic.
      /// When using this method, the user might want to use
      /// SetConnectionsCb() and SetDisconnectionCb(), that registers callbacks
      /// that will be executed when the topic address is discovered or when the
      /// node providing the topic is disconnected.
      /// \sa SetConnectionsCb.
      /// \sa SetDisconnectionsCb.
      /// \param[in] _topic Topic name requested.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool DiscoverMsg(const std::string &_topic);

      /// \brief Request discovery information about a service.
      /// The user might want to use SetConnectionsSrvCb() and
      /// SetDisconnectionSrvCb(), that registers callbacks that will be
      /// executed when the service address is discovered or when the node
      /// providing the service is disconnected.
      /// \sa SetConnectionsSrvCb.
      /// \sa SetDisconnectionsSrvCb.
      /// \param[in] _topic Topic name requested.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool DiscoverSrv(const std::string &_topic);

      /// \brief Get the discovery information object (messages).
      /// \return Reference to the discovery information object.
      public: const TopicStorage<MessagePublisher> &DiscoveryMsgInfo() const;

      /// \brief Get the discovery information object (services).
      /// \return Reference to the discovery information object.
      public: const TopicStorage<ServicePublisher> &DiscoverySrvInfo() const;

      /// \brief Get all the publishers' information known for a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _publishers Publishers requested.
      /// \return True if the topic is found and there is at least one publisher
      public: bool MsgPublishers(const std::string &_topic,
                                 MsgAddresses_M &_publishers);

      /// \brief Get all the publishers' information known for a given service.
      /// \param[in] _topic Service name.
      /// \param[out] _publishers Publishers requested.
      /// \return True if the topic is found and there is at least one publisher
      public: bool SrvPublishers(const std::string &_topic,
                                 SrvAddresses_M &_publishers);

      /// \brief Unadvertise a new message. Broadcast a discovery
      /// message that will cancel all the discovery information for the topic
      /// advertised by a specific node.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \param[in] _nUuid Node UUID.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool UnadvertiseMsg(const std::string &_topic,
                                  const std::string &_nUuid);

      /// \brief Unadvertise a new message service. Broadcast a discovery
      /// message that will cancel all the discovery information for the service
      /// advertised by a specific node.
      /// \param[in] _topic Service name to be unadvertised.
      /// \param[in] _nUuid Node UUID.
      /// \return True if the method succeed or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool UnadvertiseSrv(const std::string &_topic,
                                  const std::string &_nUuid);

      /// \brief Get the IP address of this host.
      /// \return A string with this host's IP address.
      public: std::string HostAddr() const;

      /// \brief The discovery checks the validity of the topic information
      /// every 'activity interval' milliseconds.
      /// \sa SetActivityInterval.
      /// \return The value in milliseconds.
      public: unsigned int ActivityInterval() const;

      /// \brief Each node broadcasts periodic heartbeats to keep its topic
      /// information alive in other nodes. A heartbeat message is sent after
      /// 'heartbeat interval' milliseconds.
      /// \sa SetHeartbeatInterval.
      /// \return The value in milliseconds.
      public: unsigned int HeartbeatInterval() const;

      /// \brief While a topic is being advertised by a node, a beacon is sent
      /// periodically every 'advertise interval' milliseconds.
      /// \sa SetAdvertiseInterval.
      /// \return The value in milliseconds.
      public: unsigned int AdvertiseInterval() const;

      /// \brief Get the maximum time allowed without receiving any discovery
      /// information from a node before canceling its entries.
      /// \sa SetSilenceInterval.
      /// \return The value in milliseconds.
      public: unsigned int SilenceInterval() const;

      /// \brief Set the activity interval.
      /// \sa ActivityInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetActivityInterval(const unsigned int _ms);

      /// \brief Set the heartbeat interval.
      /// \sa HeartbeatInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetHeartbeatInterval(const unsigned int _ms);

      /// \brief Set the advertise interval.
      /// \sa AdvertiseInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetAdvertiseInterval(const unsigned int _ms);

      /// \brief Set the maximum silence interval.
      /// \sa SilenceInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetSilenceInterval(const unsigned int _ms);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new topic is connected, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void ConnectionsCb(const MsgDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new topic is discovered, the callback will be executed.
      /// This version uses a member functions as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void ConnectionsCb(
        void(C::*_cb)(const MessagePublisher &_pub),
        C *_obj)
      {
        this->ConnectionsCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void DisconnectionsCb(
        const transport::MsgDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void DisconnectionsCb(
        void(C::*_cb)(const MessagePublisher &_pub),
        C *_obj)
      {
        this->DisconnectionsCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Register a callback to receive discovery connection events for
      /// services.
      /// Each time a new service is available, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void ConnectionsSrvCb(const SrvDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events for
      /// services.
      /// Each time a new service is available, the callback will be executed.
      /// This version uses a member functions as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void ConnectionsSrvCb(
        void(C::*_cb)(const ServicePublisher &_pub),
        C *_obj)
      {
        this->ConnectionsSrvCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Register a callback to receive discovery disconnection events
      /// for services.
      /// Each time a service is no longer available, the callback will be
      /// executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void DisconnectionsSrvCb(
        const transport::SrvDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a service is no longer available, the callback will be
      /// executed.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void DisconnectionsSrvCb(
        void(C::*_cb)(const ServicePublisher &_pub), C *_obj)
      {
        this->DisconnectionsSrvCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Print the current discovery state (info, activity, unknown).
      public: void PrintCurrentState() const;

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void TopicList(std::vector<std::string> &_topics) const;

      /// \brief Get the list of services currently advertised in the network.
      /// \param[out] _topics List of advertised services.
      public: void ServiceList(std::vector<std::string> &_services) const;

      /// \brief Get mutex used in the Discovery class.
      /// \return The discovery mutex.
      public: std::recursive_mutex& Mutex() const;

      /// \brief Check if ready/initialized. If not, then wait on the
      /// initializedCv condition variable.
      public: void WaitForInit() const;

      /// \brief Check the validity of the topic information. Each topic update
      /// has its own timestamp. This method iterates over the list of topics
      /// and invalids the old topics.
      private: void RunActivityTask();

      /// \brief Broadcast periodic heartbeats.
      private: void RunHeartbeatTask();

      /// \brief Receive discovery messages.
      private: void RunReceptionTask();

      /// \brief Method in charge of receiving the discovery updates.
      private: void RecvDiscoveryUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _fromIp IP address of the message sender.
      /// \param[in] _msg Received message.
      private: void DispatchDiscoveryMsg(const std::string &_fromIp,
                                         char *_msg);

      /// \brief Broadcast a discovery message.
      /// \param[in] _type Message type.
      /// \param[in] _pub Publishers's information to send.
      /// \param[in] _flags Optional flags. Currently, the flags are not used
      /// but they will in the future for specifying things like compression,
      /// or encryption.
      private: template<typename T> void SendMsg(uint8_t _type,
          const T &_pub, const uint16_t _flags = 0) const
      {
        // Create the header.
        Header header(this->Version(), _pub.PUuid(), _type, _flags);
        auto msgLength = 0;
        std::vector<char> buffer;

        std::string topic = _pub.Topic();

        switch (_type)
        {
          case AdvType:
          case UnadvType:
          case AdvSrvType:
          case UnadvSrvType:
          {
            // Create the [UN]ADVERTISE message.
            transport::AdvertiseMessage<T> advMsg(header, _pub);

            // Allocate a buffer and serialize the message.
            buffer.resize(advMsg.MsgLength());
            advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = static_cast<int>(advMsg.MsgLength());
            break;
          }
          case SubType:
          case SubSrvType:
          {
            // Create the [UN]SUBSCRIBE message.
            SubscriptionMsg subMsg(header, topic);

            // Allocate a buffer and serialize the message.
            buffer.resize(subMsg.MsgLength());
            subMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = static_cast<int>(subMsg.MsgLength());
            break;
          }
          case HeartbeatType:
          case ByeType:
          {
            // Allocate a buffer and serialize the message.
            buffer.resize(header.HeaderLength());
            header.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = header.HeaderLength();
            break;
          }
          default:
            std::cerr << "Discovery::SendMsg() error: Unrecognized message"
                      << " type [" << _type << "]" << std::endl;
            return;
        }

        // Send the discovery message to the multicast group through all the
        // sockets.
        for (const auto &sock : this->Sockets())
        {
          std::lock_guard<std::recursive_mutex> lock(this->Mutex());
          if (sendto(sock, reinterpret_cast<const raw_type *>(
            reinterpret_cast<unsigned char*>(&buffer[0])),
            msgLength, 0, reinterpret_cast<sockaddr *>(this->MulticastAddr()),
            sizeof(*(this->MulticastAddr()))) != msgLength)
          {
            std::cerr << "Exception sending a message" << std::endl;
            return;
          }
        }

        if (this->Verbose())
        {
          std::cout << "\t* Sending " << MsgTypesStr[_type]
                    << " msg [" << topic << "]" << std::endl;
        }
      }

      /// \brief Get the list of sockets used for discovery.
      /// \return The list of sockets.
      private: std::vector<int>& Sockets() const;

      /// \brief Get the data structure used for multicast communication.
      /// \return The data structure containing the multicast information.
      private: sockaddr_in* MulticastAddr() const;

      /// \brief Get the verbose mode.
      /// \return True when verbose mode is enabled or false otherwise.
      private: bool Verbose() const;

      /// \brief Get the discovery protocol version.
      /// \return The discovery version.
      private: uint8_t Version() const;

      /// \brief Register a new network interface in the discovery system.
      /// \param[in] _ip IP address to register.
      /// \return True when the interface was successfully registered or false
      /// otherwise (e.g.: invalid IP address).
      private: bool RegisterNetIface(const std::string &_ip);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<DiscoveryPrivate> dataPtr;
    };
  }
}

#endif
