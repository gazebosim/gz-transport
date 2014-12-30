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

#ifdef _WIN32
  // For socket(), connect(), send(), and recv().
  #include <Winsock2.h>
  // Type used for raw data on this platform.
  typedef char raw_type;
#else
  // For sockaddr_in
  #include <netinet/in.h>
  // Type used for raw data on this platform
  typedef void raw_type;
#endif

#include <functional>
#include <memory>
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
      /// \brief Constructor.
      /// \param[in] _pUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Discovery(const std::string &_pUuid, bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Discovery();

      /// \brief Advertise a new message.
      /// \param[in] _publisher Publisher's information to advertise.
      public: void AdvertiseMsg(const MessagePublisher &_publisher);

      /// \brief Advertise a new service.
      /// \param[in] _publisher Publisher's information to advertise.
      public: void AdvertiseSrv(const ServicePublisher &_publisher);

      /// \brief Request discovery information about a topic.
      /// When using this method, the user might want to use
      /// SetConnectionsCb() and SetDisconnectionCb(), that register callbacks
      /// that will be executed when the topic address is discovered or when the
      /// node providing the topic is disconnected.
      /// \sa SetConnectionsCb.
      /// \sa SetDisconnectionsCb.
      /// \param[in] _topic Topic name requested.
      public: void DiscoverMsg(const std::string &_topic);

      /// \brief Request discovery information about a service.
      /// The user might want to use SetConnectionsSrvCb() and
      /// SetDisconnectionSrvCb(), that register callbacks that will be executed
      /// when the service address is discovered or when the node providing the
      /// service is disconnected.
      /// \sa SetConnectionsSrvCb.
      /// \sa SetDisconnectionsSrvCb.
      /// \param[in] _topic Topic name requested.
      public: void DiscoverSrv(const std::string &_topic);

      public: TopicStorage<MessagePublisher>& GetDiscoveryMsgInfo() const;

      public: TopicStorage<ServicePublisher>& GetDiscoverySrvInfo() const;

      /// \brief Get all the publisher's known for a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _publishers Publishers requested.
      /// \return True if the topic is found and there is at least one publisher
      public: bool GetMsgPublishers(const std::string &_topic,
                                    MsgAddresses_M &_publishers);

      /// \brief Get all the publishers known for a given service.
      /// \param[in] _topic Service name.
      /// \param[out] _publishers Publishers requested.
      /// \return True if the topic is found and there is at least one publisher
      public: bool GetSrvPublishers(const std::string &_topic,
                                    SrvAddresses_M &_publishers);

      /// \brief Unadvertise a new message. Broadcast a discovery
      /// message that will cancel all the discovery information for the topic
      /// advertised by a specific node.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \param[in] _nUuid Node UUID.
      public: void UnadvertiseMsg(const std::string &_topic,
                                  const std::string &_nUuid);

      /// \brief Unadvertise a new message service. Broadcast a discovery
      /// message that will cancel all the discovery information for the service
      /// advertised by a specific node.
      /// \param[in] _topic Service name to be unadvertised.
      /// \param[in] _nUuid Node UUID.
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
      public: void SetConnectionsCb(const MsgDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new topic is discovered, the callback will be executed.
      /// This version uses a member functions as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetConnectionsCb(
        void(C::*_cb)(const MessagePublisher &_pub),
        C *_obj)
      {
        this->SetConnectionsCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetDisconnectionsCb(
        const transport::MsgDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetDisconnectionsCb(
        void(C::*_cb)(const MessagePublisher &_pub),
        C *_obj)
      {
        this->SetDisconnectionsCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Register a callback to receive discovery connection events for
      /// services.
      /// Each time a new service is available, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetConnectionsSrvCb(const SrvDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery connection events for
      /// services.
      /// Each time a new service is available, the callback will be executed.
      /// This version uses a member functions as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetConnectionsSrvCb(
        void(C::*_cb)(const ServicePublisher &_pub),
        C *_obj)
      {
        this->SetConnectionsSrvCb(std::bind(_cb, _obj, std::placeholders::_1));
      }

      /// \brief Register a callback to receive discovery disconnection events
      /// for services.
      /// Each time a service is no longer available, the callback will be
      /// executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void SetDisconnectionsSrvCb(
        const transport::SrvDiscoveryCallback &_cb);

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a service is no longer available, the callback will be
      /// executed.
      /// This version uses a member function as callback.
      /// \param[in] _cb Function callback with the following parameters.
      ///                _pub Publisher's information.
      /// \param[in] _obj Object instance where the member function belongs.
      public: template<typename C> void SetDisconnectionsSrvCb(
        void(C::*_cb)(const ServicePublisher &_pub), C *_obj)
      {
        this->SetDisconnectionsSrvCb(
          std::bind(_cb, _obj, std::placeholders::_1));
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

      /// \brief Get the socket used for sending/receiving discovery messages.
      /// \return Discovery socket.
      private: int DiscoverySocket() const;

      /// \brief Get the data structure used for multicast communication.
      /// \return The data structure containing the multicast information.
      private: sockaddr_in* MulticastAddr() const;

      /// \brief Get the verbose mode.
      /// \return True when verbose mode is enable or false otherwise.
      private: bool Verbose() const;

      /// \brief Get the discovery protocol version.
      /// \return The discovery version.
      private: uint8_t Version() const;

      /// \brief Broadcast a discovery message.
      /// \param[in] _type Message type.
      /// \param[in] _pub Publishers's information to send.
      /// \param[in] _flags Optional flags. Currently, the flags are not used
      /// but they will in the future for specifying things like compression,
      /// or encryption.
      public: template<typename T> void SendMsg(uint8_t _type,
                                                const T &_pub,
                                                int _flags = 0)
      {
        // Create the header.
        Header header(this->Version(), _pub.PUuid(), _type, _flags);
        auto msgLength = 0;
        std::vector<char> buffer;

        std::string _topic = _pub.Topic();

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
            buffer.resize(advMsg.GetMsgLength());
            advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = advMsg.GetMsgLength();
            break;
          }
          case SubType:
          case SubSrvType:
          {
            // Create the [UN]SUBSCRIBE message.
            SubscriptionMsg subMsg(header, _topic);

            // Allocate a buffer and serialize the message.
            buffer.resize(subMsg.GetMsgLength());
            subMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = subMsg.GetMsgLength();
            break;
          }
          case HeartbeatType:
          case ByeType:
          {
            // Allocate a buffer and serialize the message.
            buffer.resize(header.GetHeaderLength());
            header.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = header.GetHeaderLength();
            break;
          }
          default:
            std::cerr << "Discovery::SendMsg() error: Unrecognized message"
                      << " type [" << _type << "]" << std::endl;
            return;
        }

        // Send the discovery message to the multicast group.
        if (sendto(this->DiscoverySocket(), reinterpret_cast<const raw_type *>(
          reinterpret_cast<unsigned char*>(&buffer[0])),
          msgLength, 0, reinterpret_cast<sockaddr *>(this->MulticastAddr()),
          sizeof(*(this->MulticastAddr()))) != msgLength)
        {
          std::cerr << "Exception sending a message" << std::endl;
          return;
        }

        if (this->Verbose())
        {
          std::cout << "\t* Sending " << MsgTypesStr[_type]
                    << " msg [" << _topic << "]" << std::endl;
        }
      }

      /// \brief Print the current discovery state (info, activity, unknown).
      public: void PrintCurrentState();

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void GetTopicList(std::vector<std::string> &_topics) const;

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void GetServiceList(std::vector<std::string> &_services) const;

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<DiscoveryPrivate> dataPtr;
    };
  }
}
#endif
