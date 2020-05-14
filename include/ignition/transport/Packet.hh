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

#ifndef IGN_TRANSPORT_PACKET_HH_
#define IGN_TRANSPORT_PACKET_HH_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "ignition/transport/config.hh"
#include "ignition/transport/Export.hh"
#include "ignition/transport/Publisher.hh"

// This whole file is deprecated in version 8 of Ignition Transport. Please
// remove this file in Version 9.

namespace ignition
{
  namespace transport
  {
    // Inline bracket to help doxygen filtering.
    inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
    //
    // Message types.
    static const uint8_t Uninitialized  = 0;
    static const uint8_t AdvType        = 1;
    static const uint8_t SubType        = 2;
    static const uint8_t UnadvType      = 3;
    static const uint8_t HeartbeatType  = 4;
    static const uint8_t ByeType        = 5;
    static const uint8_t NewConnection  = 6;
    static const uint8_t EndConnection  = 7;

    // Flag set when a discovery message is relayed.
    static const uint16_t FlagRelay   = 0b000000000000'0001;
    // Flag set when we want to avoid to relay a discovery message.
    // This is used to avoid loops.
    static const uint16_t FlagNoRelay = 0b000000000000'0010;

    /// \brief Used for debugging the message type received/send.
    static const std::vector<std::string> MsgTypesStr =
    {
      "UNINITIALIZED", "ADVERTISE", "SUBSCRIBE", "UNADVERTISE", "HEARTBEAT",
      "BYE", "NEW_CONNECTION", "END_CONNECTION"
    };

    /// \class Header Packet.hh ignition/transport/Packet.hh
    /// \brief Header included in each discovery message containing the version
    /// of the discovery protocol, the process UUID of the sender node, the type
    /// of message (ADV, SUB, ... ) and optional flags.
    /// \deprecated This class is deprecated. Discovery uses the
    /// ignition::msgs::Discovery message.
    class IGNITION_TRANSPORT_VISIBLE Header
    {
      /// \brief Constructor.
      public: IGN_DEPRECATED(8) Header() = default;

      /// \brief Constructor.
      /// \param[in] _version Version of the discovery protocol.
      /// \param[in] _pUuid Every process has a unique UUID.
      /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
      /// \param[in] _flags Optional flags included in the header.
      public: IGN_DEPRECATED(8) Header(const uint16_t _version,
                     const std::string &_pUuid,
                     const uint8_t _type,
                     const uint16_t _flags = 0);

      /// \brief Destructor.
      public: virtual ~Header() = default;

      /// \brief Get the discovery protocol version.
      /// \return The discovery protocol version.
      /// \sa SetVersion.
      public: uint16_t Version() const;

      /// \brief Get the process uuid.
      /// \return A unique global identifier for every process.
      /// \sa SetPUuid.
      public: std::string PUuid() const;

      /// \brief Get the message type.
      /// \return Message type (ADVERTISE, SUBSCRIPTION, ...)
      /// \sa SetType.
      public: uint8_t Type() const;

      /// \brief Get the message flags.
      /// \return Message flags used for compression or other optional features.
      /// \sa SetFlags.
      public: uint16_t Flags() const;

      /// \brief Set the discovery protocol version.
      /// \param[in] _version Discovery protocol version.
      /// \sa Version.
      public: void SetVersion(const uint16_t _version);

      /// \brief Set the process uuid.
      /// \param[in] _pUuid A unique global identifier for every process.
      /// \sa PUuid.
      public: void SetPUuid(const std::string &_pUuid);

      /// \brief Set the message type.
      /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...).
      /// \sa Type.
      public: void SetType(const uint8_t _type);

      /// \brief Set the message flags.
      /// \param[in] _flags Used for enable optional features.
      /// \sa Flags.
      public: void SetFlags(const uint16_t _flags);

      /// \brief Get the header length.
      /// \return The header length in bytes.
      public: int HeaderLength() const;

      /// \brief Serialize the header. The caller has ownership of the
      /// buffer and is responsible for its [de]allocation.
      /// \param[out] _buffer Destination buffer in which the header
      /// will be serialized.
      /// \return Number of bytes serialized.
      public: size_t Pack(char *_buffer) const;

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
             << "\tVersion: " << _header.Version() << "\n"
             << "\tProcess UUID: " << _header.PUuid() << "\n"
             << "\tType: " << MsgTypesStr.at(_header.Type()) << "\n"
             << "\tFlags: " << _header.Flags() << "\n";
        return _out;
      }

      /// \brief Discovery protocol version.
      private: uint16_t version = 0;

#ifdef _WIN32
// Disable warning C4251 which is triggered by
// std::string
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
      /// \brief Global identifier. Every process has a unique UUID.
      private: std::string pUuid = "";
#ifdef _WIN32
#pragma warning(pop)
#endif

      /// \brief Message type (ADVERTISE, SUBSCRIPTION, ...).
      private: uint8_t type = Uninitialized;

      /// \brief Optional flags that you want to include in the header.
      private: uint16_t flags = 0;
    };

    /// \class SubscriptionMsg Packet.hh ignition/transport/Packet.hh
    /// \brief Subscription packet used in the discovery protocol for requesting
    /// information about a given topic.
    /// \deprecated This class is deprecated. Discovery uses the
    /// ignition::msgs::Discovery message.
    class IGNITION_TRANSPORT_VISIBLE SubscriptionMsg
    {
      /// \brief Constructor.
      public: IGN_DEPRECATED(8) SubscriptionMsg() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      public: IGN_DEPRECATED(8) SubscriptionMsg(
                  const transport::Header &_header,
                  const std::string &_topic);

      /// \brief Get the message header.
      /// \return Reference to the message header.
      /// \sa SetHeader.
      public: transport::Header Header() const;

      /// \brief Get the topic.
      /// \return Topic name.
      /// \sa SetTopic.
      public: std::string Topic() const;

      /// \brief Set the header of the message.
      /// \param[in] _header Message header.
      /// \sa Header.
      public: void SetHeader(const transport::Header &_header);

      /// \brief Set the topic.
      /// \param[in] _topic Topic name.
      /// \sa Topic.
      public: void SetTopic(const std::string &_topic);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t MsgLength() const;

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg SubscriptionMsg message to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const SubscriptionMsg &_msg)
      {
        _out << _msg.Header()
             << "Body:" << std::endl
             << "\tTopic: [" << _msg.Topic() << "]" << std::endl;

        return _out;
      }

      /// \brief Serialize the subscription message.
      /// \param[out] _buffer Buffer where the message will be serialized.
      /// \return The length of the serialized message in bytes.
      public: size_t Pack(char *_buffer) const;

      /// \brief Unserialize a stream of bytes into a Sub.
      /// \param[out] _buffer Unpack the body from the buffer.
      /// \return The number of bytes from the body.
      public: size_t Unpack(const char *_buffer);

      /// \brief Message header.
      private: transport::Header header;

#ifdef _WIN32
// Disable warning C4251 which is triggered by std::string
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
      /// \brief Topic.
      private: std::string topic = "";
#ifdef _WIN32
#pragma warning(pop)
#endif
    };

    /// \class AdvertiseMessage Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise packet used in the discovery protocol to broadcast
    /// information about the node advertising a topic. The information sent
    /// contains the name of the protobuf message type advertised. This message
    /// is used for advertising messages and services. 'T' is the Publisher
    /// type used inside this AdvertiseMessage object.
    /// \deprecated This class is deprecated. Discovery uses the
    /// ignition::msgs::Discovery message.
    template <class T> class AdvertiseMessage
    {
      /// \brief Constructor.
      public: IGN_DEPRECATED(8) AdvertiseMessage() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _publisher Contains the topic name, UUIDs, addresses.
      public: IGN_DEPRECATED(8) AdvertiseMessage(const Header &_header,
                               const T &_publisher)
        : header(_header),
          publisher(_publisher)
      {
      }

      /// \brief Get the message header.
      /// \return Reference to the message header.
      /// \sa SetHeader.
      public: transport::Header Header() const
      {
        return this->header;
      }

      /// \brief Get the publisher of this message.
      /// \return Publisher.
      /// \sa SetPublisher.
      public: T& Publisher()
      {
        return this->publisher;
      }

      /// \brief Set the header of the message.
      /// \param[in] _header Message header.
      /// \sa Header.
      public: void SetHeader(const transport::Header &_header)
      {
        this->header = _header;
      }

      /// \brief Set the publisher of this message.
      /// \param[in] _publisher New publisher.
      /// \sa Publisher.
      public: void SetPublisher(const T &_publisher)
      {
        this->publisher = _publisher;
      }

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t MsgLength() const
      {
        return this->header.HeaderLength() + this->publisher.MsgLength();
      }

      /// \brief Serialize the advertise message.
      /// \param[out] _buffer Buffer where the message will be serialized.
      /// \return The length of the serialized message in bytes.
      public: size_t Pack(char *_buffer) const
      {
        // Pack the common part of any advertise message.
        size_t len = this->header.Pack(_buffer);
        if (len == 0)
          return 0;

        _buffer += len;

        // Pack the part of the publisher.
        if (this->publisher.Pack(_buffer) == 0)
          return 0;

        return this->MsgLength();
      }

      /// \brief Unserialize a stream of bytes into an AdvertiseMessage.
      /// \param[out] _buffer Unpack the body from the buffer.
      /// \return The number of bytes from the body.
      public: size_t Unpack(const char *_buffer)
      {
        // Unpack the message publisher.
        if (this->publisher.Unpack(_buffer) == 0)
          return 0;

        return this->publisher.MsgLength();
      }

      /// \brief Set from discovery message.
      /// \param[in] _msg Discovery message.
      public: void SetFromDiscovery(const msgs::Discovery &_msg)
      {
        this->publisher.SetFromDiscovery(_msg);
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
      private: transport::Header header;

      /// \brief Publisher information (topic, ZMQ address, UUIDs, etc.).
      private: T publisher;
    };
    }
  }
}

#endif
