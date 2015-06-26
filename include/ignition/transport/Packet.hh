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
#include "msgs/packet.pb.h"

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
    /// of the discovery protocol, the process UUID of the sender node, the type
    // of message (ADV, SUB, ... ) and optional flags.
    class IGNITION_VISIBLE Header
    {
      /// \brief Constructor.
      public: Header() = default;

      /// \brief Constructor.
      /// \param[in] _version Version of the discovery protocol.
      /// \param[in] _pUuid Every process has a unique UUID.
      /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...)
      /// \param[in] _partition Partition name.
      /// \param[in] _flags Optional flags included in the header.
      public: Header(const uint16_t _version,
                     const std::string &_pUuid,
                     const uint8_t _type,
                     const uint16_t _flags = 0);

      /// \brief Destructor.
      public: virtual ~Header() = default;

      /// \brief Get the discovery protocol version.
      /// \return The discovery protocol version.
      public: uint16_t Version() const;

      /// \brief Get the process uuid.
      /// \return A unique global identifier for every process.
      public: std::string PUuid() const;

      /// \brief Get the message type.
      /// \return Message type (ADVERTISE, SUBSCRIPTION, ...)
      public: uint8_t Type() const;

      /// \brief Get the message flags.
      /// \return Message flags used for compression or other optional features.
      public: uint16_t Flags() const;

      /// \brief Set the discovery protocol version.
      /// \param[in] _version Discovery protocol version.
      public: void Version(const uint16_t _version);

      /// \brief Set the process uuid.
      /// \param[in] _pUuid A unique global identifier for every process.
      public: void PUuid(const std::string &_pUuid);

      /// \brief Set the message type.
      /// \param[in] _type Message type (ADVERTISE, SUBSCRIPTION, ...).
      public: void Type(const uint8_t _type);

      /// \brief Set the message flags.
      /// \param[in] _flags Used for enable optional features.
      public: void Flags(const uint16_t _flags);

      /// \brief Serialize the header.
      /// \param[out] _buffer Buffer where the message will be serialized.
      /// \return Bool whether everything went ok.
      public: virtual bool Pack(std::vector<char> &_buffer) const;

      /// \brief Unserialize a stream of bytes into a header.
      /// \param[in] _buffer Unpack the body from the buffer.
      /// \return Bool whether everything went ok.
      public: virtual bool Unpack(const std::vector<char> &_buffer);

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

      /// \brief Global identifier. Every process has a unique guid.
      private: std::string pUuid = "";

      /// \brief Message type (ADVERTISE, SUBSCRIPTION, ...).
      private: uint8_t type = Uninitialized;

      /// \brief Optional flags that you want to include in the header.
      private: uint16_t flags = 0;
    };

    /// \class Message Packet.hh ignition/transport/Packet.hh
    /// \brief Message base class.
    class IGNITION_VISIBLE Message
    {
      /// \brief Constructor.
      public: Message() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      public: Message(const Header &_header);

      /// \brief Get the message header.
      /// \return Copy of the message header.
      public: Header GetHeader() const;

      /// \brief Set the header of the message.
      /// \param[in] _header Message header.
      public: void SetHeader(const Header &_header);

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg Message to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const Message &_msg)
      {
        _out << _msg.GetHeader() << std::endl;

        return _out;
      }

      /// \brief Serialize the subscription message.
      /// \param[out] _buffer Buffer where the message will be serialized.
      /// \return Bool whether everything went ok.
      public: virtual bool Pack(std::vector<char> &_buffer) const;

      /// \brief Unserialize a stream of bytes into a Sub.
      /// \param[in] _buffer Unpack the body from the buffer.
      /// \return Bool whether everything went ok.
      public: virtual bool Unpack(const std::vector<char> &_buffer);

      /// \brief Message header.
      protected: Header header;
    };

    /// \class SubscriptionMsg Packet.hh ignition/transport/Packet.hh
    /// \brief Subscription packet used in the discovery protocol for requesting
    /// information about a given topic.
    class IGNITION_VISIBLE SubscriptionMsg : public Message
    {
      /// \brief Constructor.
      public: SubscriptionMsg() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      public: SubscriptionMsg(const Header &_header,
                              const std::string &_topic);

      /// \brief Get the topic.
      /// \return Topic name.
      public: std::string Topic() const;

      /// \brief Set the topic.
      /// \param[in] _topic Topic name.
      public: void Topic(const std::string &_topic);

      // Documentation inherited.
      public: bool Pack(std::vector<char> &_buffer) const override;

      // Documentation inherited.
      public: bool Unpack(const std::vector<char> &_buffer) override;

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg SubscriptionMsg message to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const SubscriptionMsg &_msg)
      {
        _out << _msg.GetHeader()
             << "Body:" << std::endl
             << "\tTopic: [" << _msg.Topic() << "]" << std::endl;

        return _out;
      }

      /// \brief Topic.
      private: std::string topic = "";
    };

    /// \class AdvertiseMessage Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise packet used in the discovery protocol to broadcast
    /// information about the node advertising a topic. The information sent
    /// contains the name of the protobuf message type advertised. This message
    /// is used for advertising messages and services. 'T' is the Publisher
    /// type used inside this AdvertiseMessage object.

    template <class T> class IGNITION_VISIBLE AdvertiseMessage : public Message
    {
      /// \brief Constructor.
      public: AdvertiseMessage() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _publisher Contains the topic name, UUIDs, addresses.
      public: AdvertiseMessage(const Header &_header,
                               const T &_publisher)
        : Message(_header),
          publisher(_publisher)
      {
      }

      /// \brief Get the publisher of this message.
      /// \return Publisher.
      public: T& GetPublisher()
      {
        return this->publisher;
      }

      /// \brief Set the publisher of this message.
      /// \param[in] _publisher New publisher.
      public: void SetPublisher(const T &_publisher)
      {
        this->publisher = _publisher;
      }

      // Documentation inherited.
      public: bool Pack(std::vector<char> &_buffer) const override
      {
        if (!Message::Pack(_buffer))
          return false;

        msgs::MessageData message;
        if (!unserialize(_buffer, message))
          return false;

        std::vector<char> v;
        if (!this->publisher.Pack(v))
        {
          std::cerr << "AdvertiseMessage::Pack() error "
                    << "packing publisher" << std::endl;
          return false;
        }

        msgs::PublisherData pubMsg;
        if (!unserialize(v, pubMsg))
          return false;
        message.mutable_publisher()->CopyFrom(pubMsg);

        return serialize(message, _buffer);
      }

      // Documentation inherited.
      public: bool Unpack(const std::vector<char> &_buffer) override
      {
        // Empty buffer.
        if (_buffer.empty())
        {
          std::cerr << "AdvertiseMessage::Unpack() error: Empty input buffer"
                    << std::endl;
          return false;
        }

        if (!Message::Unpack(_buffer))
          return false;

        msgs::MessageData message;
        if (!unserialize(_buffer, message))
          return false;

        std::vector<char> publisherBuffer;
        if (!serialize(message.publisher(), publisherBuffer))
          return false;

        return this->publisher.Unpack(publisherBuffer);
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

      /// \brief Publisher information (topic, ZMQ address, UUIDs, etc.).
      private: T publisher;
    };
  }
}

#endif
