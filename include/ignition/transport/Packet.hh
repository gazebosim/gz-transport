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

#ifndef __IGN_TRANSPORT_PACKET_HH_INCLUDED__
#define __IGN_TRANSPORT_PACKET_HH_INCLUDED__

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    // Message types.
    static const uint8_t Uninitialized  = 0;
    static const uint8_t AdvType        = 1;
    static const uint8_t SubType        = 2;
    static const uint8_t UnadvType      = 3;
    static const uint8_t HeartbeatType  = 4;
    static const uint8_t ByeType        = 5;
    static const uint8_t AdvSrvType     = 6;
    static const uint8_t SubSrvType     = 7;
    static const uint8_t UnadvSrvType   = 8;
    static const uint8_t NewConnection  = 9;
    static const uint8_t EndConnection  = 10;

    /// \brief Used for debugging the message type received/send.
    static const std::vector<std::string> MsgTypesStr =
    {
      "UNINITIALIZED", "ADVERTISE", "SUBSCRIBE", "UNADVERTISE", "HEARTBEAT",
      "BYE", "ADV_SRV", "SUB_SRV", "UNADVERTISE_SRV", "NEW_CONNECTION",
      "END_CONNECTION"
    };

    /// \class Header Packet.hh ignition/transport/Packet.hh
    /// \brief Header included in each discovery message containing the version
    /// of the discovery protocol, the process UUID of the sender node, the
    /// topic contained in the message, the type of message (ADV, SUB, ... ) and
    /// optional flags.
    class IGNITION_VISIBLE Header
    {
      /// \brief Constructor.
      public: Header() = default;

      /// \brief Constructor.
      /// \param[in] _version Version of the discovery protocol.
      /// \param[in] _pUuid Every process has a unique UUID.
      /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
      /// \param[in] _flags Optional flags included in the header.
      public: Header(const uint16_t _version,
                     const std::string &_pUuid,
                     const uint8_t _type,
                     const uint16_t _flags = 0);

      public: Header(const Header &_header);

      /// \brief Destructor.
      public: virtual ~Header() = default;

      /// \brief Get the discovery protocol version.
      /// \return The discovery protocol version.
      public: uint16_t GetVersion() const;

      /// \brief Get the process uuid.
      /// \return A unique global identifier for every process.
      public: std::string GetPUuid() const;

      /// \brief Get the message type.
      /// \return Message type (ADVERTISE, SUBSCRIPTION, ...)
      public: uint8_t GetType() const;

      /// \brief Get the message flags.
      /// \return Message flags used for compression or other optional features.
      public: uint16_t GetFlags() const;

      /// \brief Set the discovery protocol version.
      /// \param[in] _version Discovery protocol version.
      public: void SetVersion(const uint16_t _version);

      /// \brief Set the process uuid.
      /// \param[in] _pUuid A unique global identifier for every process.
      public: void SetPUuid(const std::string &_pUuid);

      /// \brief Set the message type.
      /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...).
      public: void SetType(const uint8_t _type);

      /// \brief Set the message flags.
      /// \param[in] _flags Used for enable optional features.
      public: void SetFlags(const uint16_t _flags);

      /// \brief Get the header length.
      /// \return The header length in bytes.
      public: int GetHeaderLength();

      /// \brief Serialize the header. The caller has ownership of the
      /// buffer and is responsible for its [de]allocation.
      /// \param[out] _buffer Destination buffer in which the header
      /// will be serialized.
      /// \return Number of bytes serialized.
      public: size_t Pack(char *_buffer);

      /// \brief Unserialize the header.
      /// \param[in] _buffer Input buffer with the data to be unserialized.
      public: size_t Unpack(const char *_buffer);

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg Header to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const Header &_header)
      {
        _out << "--------------------------------------\n"
             << "Header:" << std::endl
             << "\tVersion: " << _header.GetVersion() << "\n"
             << "\tProcess UUID: " << _header.GetPUuid() << "\n"
             << "\tType: " << MsgTypesStr.at(_header.GetType()) << "\n"
             << "\tFlags: " << _header.GetFlags() << "\n";
        return _out;
      }

      /// \brief Discovery protocol version.
      private: uint16_t version = 0;

      /// \brief Global identifier. Every process has a unique guid.
      private: std::string pUuid = "";

      /// \brief Message type (ADVERTISE, SUBSCRIPTION, ...).
      private: uint8_t type = Uninitialized;

      /// \brief Optional flags that you want to include in the header.
      private: uint16_t flags = 0;
    };

    /// \class SubscriptionMsg Packet.hh ignition/transport/Packet.hh
    /// \brief Subscription packet used in the discovery protocol for requesting
    /// information about a given topic.
    class IGNITION_VISIBLE SubscriptionMsg
    {
      /// \brief Constructor.
      public: SubscriptionMsg() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      public: SubscriptionMsg(const Header &_header,
                              const std::string &_topic);

      /// \brief Get the message header.
      /// \return Reference to the message header.
      public: Header GetHeader() const;

      /// \brief Get the topic.
      /// \return Topic name.
      public: std::string GetTopic() const;

      /// \brief Set the header of the message.
      /// \param[in] _header Message header.
      public: void SetHeader(const Header &_header);

      /// \brief Set the topic.
      /// \param[in] _topic Topic name.
      public: void SetTopic(const std::string &_topic);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t GetMsgLength();

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg SubscriptionMsg message to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const SubscriptionMsg &_msg)
      {
        _out << _msg.GetHeader()
             << "Body:" << std::endl
             << "\tTopic: [" << _msg.GetTopic() << "]" << std::endl;

        return _out;
      }

      /// \brief Serialize the subscription message.
      /// \param[out] _buffer Buffer where the message will be serialized.
      /// \return The length of the serialized message in bytes.
      public: size_t Pack(char *_buffer);

      /// \brief Unserialize a stream of bytes into a Sub.
      /// \param[out] _buffer Unpack the body from the buffer.
      /// \return The number of bytes from the body.
      public: size_t Unpack(char *_buffer);

      /// \brief Message header.
      private: Header header;

      /// \brief Topic.
      private: std::string topic = "";
    };

    /// \class AdvertiseMsg Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise packet used in the discovery protocol to broadcast
    /// information about the node advertising a topic. The information sent
    /// contains the name of the protobuf message type advertised.
    template <class T> class IGNITION_VISIBLE AdvertiseMessage
    {
      /// \brief Constructor.
      public: AdvertiseMessage() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address (e.g., "tcp://10.0.0.1:6000").
      /// \param[in] _ctrl ZeroMQ control address.
      /// \param[in] _nUuid Node's UUID.
      /// \param[in] _scope Topic scope.
      /// \param[in] _msgTypeName Name of the protobuf message advertised.
      public: AdvertiseMessage(const Header &_header,
                               const T &_publisher)
        : header(_header),
          publisher(_publisher)
      {
      }

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t GetMsgLength()
      {
        return this->header.GetHeaderLength() + this->publisher.GetMsgLength();
      }

      // Documentation inherited.
      public: size_t Pack(char *_buffer)
      {
        // Pack the common part of any advertise message.
        size_t len = this->header.Pack(_buffer);
        if (len == 0)
          return 0;

        _buffer += len;

        len = this->publisher.Pack(_buffer);
        if (len == 0)
          return 0;

        return this->GetMsgLength();
      }

      // Documentation inherited.
      public: size_t Unpack(char *_buffer)
      {
        // Unpack the message publisher.
        size_t len = this->publisher.Unpack(_buffer);
        if (len == 0)
          return 0;

        return this->publisher.GetMsgLength();
      }

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg AdvertiseMsg to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const AdvertiseMessage &_msg)
      {
        _out << _msg.header << _msg.publisher;

        return _out;
      }

      /// \brief The name of the protobuf message advertised.
      public: Header header;

      public: T publisher;
    };

    /// \class AdvertiseSrv Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise packet used in the discovery protocol to broadcast
    /// information about the node advertising a service. The information sent
    /// contains the name of the protobuf messages advertised for the service
    /// request and service response.
    /*class IGNITION_VISIBLE AdvertiseSrv
    {
      /// \brief Constructor.
      public: AdvertiseSrv() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address (e.g., "tcp://10.0.0.1:6000").
      /// \param[in] _ctrl ZeroMQ control address.
      /// \param[in] _nUuid Node's UUID.
      /// \param[in] _scope Topic scope.
      /// \param[in] _reqTypeName Name of the request's protobuf message.
      /// \param[in] _repTypeName Name of the response's protobuf message.
      public: AdvertiseSrv(const Header &_header,
                           const ServicePublisher &_publisher);

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg ServiceMsg to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const AdvertiseSrv &_msg)
      {
        _out << _msg.header << _msg.publisher;

        return _out;
      }

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t GetMsgLength();

      // Documentation inherited.
      public: size_t Pack(char *_buffer);

      // Documentation inherited.
      public: size_t Unpack(char *_buffer);

      public: Header header;

      public: ServicePublisher publisher;
    };*/
  }
}

#endif
