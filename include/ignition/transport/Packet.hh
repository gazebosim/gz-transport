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
#include "ignition/transport/TransportTypes.hh"
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

      /// \brief Get the header length.
      /// \return The header length in bytes.
      public: int HeaderLength();

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
      public: Header Header() const;

      /// \brief Get the topic.
      /// \return Topic name.
      public: std::string Topic() const;

      /// \brief Set the header of the message.
      /// \param[in] _header Message header.
      public: void Header(const Header &_header);

      /// \brief Set the topic.
      /// \param[in] _topic Topic name.
      public: void Topic(const std::string &_topic);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t MsgLength();

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
      public: size_t UnpackBody(char *_buffer);

      /// \brief Message header.
      private: Header header;

      /// \brief Topic.
      private: std::string topic = "";
    };

    /// \class AdvertiseBase Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise base message used as part of an advertise message or an
    /// advertise service. It stores information about the node's address
    /// advertising the message/service, its control address, node UUID and
    /// topic scope.
    class IGNITION_VISIBLE AdvertiseBase
    {
      /// \brief Constructor.
      protected: AdvertiseBase() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address (e.g., "tcp://10.0.0.1:6000").
      /// \param[in] _ctrl ZeroMQ control address.
      /// \param[in] _nUuid Node's UUID.
      /// \param[in] _scope Topic scope.
      protected: AdvertiseBase(const Header &_header,
                               const std::string &_topic,
                               const std::string &_addr,
                               const std::string &_ctrl,
                               const std::string &_nUuid,
                               const Scope &_scope);

      /// \brief Get the message header.
      /// \return Reference to the message header.
      public: Header Header() const;

      /// \brief Get the topic.
      /// \return Topic name.
      public: std::string Topic() const;

      /// \brief Get the ZMQ address.
      /// \return Return the ZMQ address.
      public: std::string Address() const;

      /// \brief Get the ZMQ control address.
      /// \return Return the ZMQ control address.
      public: std::string ControlAddress() const;

      /// \brief Get the node UUID.
      /// \return Return the node UUID.
      public: std::string NodeUuid() const;

      /// \brief Get the topic scope.
      /// \return Return the topic scope.
      public: Scope Scope() const;

      /// \brief Set the header of the message.
      /// \param[in] _header Message header.
      public: void Header(const Header &_header);

      /// \brief Set the topic.
      /// \param[in] _topic Topic name.
      public: void Topic(const std::string &_topic);

      /// \brief Set the ZMQ address.
      /// \param[in] _addr ZMQ address to be contained in the message.
      public: void Address(const std::string &_addr);

      /// \brief Set the ZMQ control address.
      /// \param[in] _ctrl ZMQ control address to be contained in the message.
      public: void ControlAddress(const std::string &_ctrl);

      /// \brief Set the node UUID.
      /// \param[in] _nUuid Node UUID.
      public: void NodeUuid(const std::string &_nUuid);

      /// \brief Set the topic scope.
      /// \param[in] _scope Topic scope.
      public: void Scope(const Scope &_scope);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t MsgLength();

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg AdvertiseBase to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const AdvertiseBase &_msg)
      {
        _out << _msg.GetHeader()
             << "Body:" << std::endl
             << "\tTopic: [" << _msg.GetTopic() << "]\n"
             << "\tAddress: " << _msg.GetAddress() << std::endl
             << "\tControl address: " << _msg.GetControlAddress() << std::endl
             << "\tNode UUID: " << _msg.GetNodeUuid() << std::endl
             << "\tTopic Scope: ";
        if (_msg.GetScope() == Scope::Process)
          _out << "Process" << std::endl;
        else if (_msg.GetScope() == Scope::Host)
          _out << "Host" << std::endl;
        else
          _out << "All" << std::endl;

        return _out;
      }

      /// \brief Serialize the message.
      /// \param[out] _buffer Buffer where the message will be serialized.
      /// \return The length of the serialized message in bytes.
      public: size_t Pack(char *_buffer);

      /// \brief Unserialize a stream of bytes into an AdvertiseBase.
      /// \param[out] _buffer Unpack the body from the buffer.
      /// \return The number of bytes from the body.
      public: size_t UnpackBody(char *_buffer);

      /// \brief Message header.
      private: Header header;

      /// \brief Topic.
      private: std::string topic = "";

      /// \brief ZMQ valid address (e.g., "tcp://10.0.0.1:6000").
      private: std::string addr = "";

      /// \brief ZMQ valid address (e.g., "tcp://10.0.0.1:6000").
      private: std::string ctrl = "";

      /// \brief Node's UUID.
      private: std::string nUuid = "";

      // Topic scope;
      private: Scope scope = Scope::All;
    };

    /// \class AdvertiseMsg Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise packet used in the discovery protocol to broadcast
    /// information about the node advertising a topic. The information sent
    /// contains the name of the protobuf message type advertised.
    class IGNITION_VISIBLE AdvertiseMsg : public AdvertiseBase
    {
      /// \brief Constructor.
      public: AdvertiseMsg() = default;

      /// \brief Constructor.
      /// \param[in] _header Message header.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address (e.g., "tcp://10.0.0.1:6000").
      /// \param[in] _ctrl ZeroMQ control address.
      /// \param[in] _nUuid Node's UUID.
      /// \param[in] _scope Topic scope.
      /// \param[in] _msgTypeName Name of the protobuf message advertised.
      public: AdvertiseMsg(const Header &_header,
                           const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_nUuid,
                           const Scope &_scope,
                           const std::string &_msgTypeName);

      /// \brief Get the name of the protobuf message advertised.
      /// \return The protobuf message type.
      public: std::string MsgTypeName() const;

      /// \brief Set the name of the protobuf message advertised.
      /// \param[in] The protobuf message type.
      public: void MsgTypeName(const std::string &_msgTypeName);

      // Documentation inherited.
      public: size_t MsgLength();

      // Documentation inherited.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const AdvertiseMsg &_msg)
      {
         _out << static_cast<const AdvertiseBase&>(_msg)
              << "\tMessage type: " << _msg.GetMsgTypeName() << std::endl;

        return _out;
      }

      // Documentation inherited.
      public: size_t Pack(char *_buffer);

      // Documentation inherited.
      public: size_t UnpackBody(char *_buffer);

      /// \brief The name of the protobuf message advertised.
      private: std::string msgTypeName = "";
    };

    /// \class AdvertiseSrv Packet.hh ignition/transport/Packet.hh
    /// \brief Advertise packet used in the discovery protocol to broadcast
    /// information about the node advertising a service. The information sent
    /// contains the name of the protobuf messages advertised for the service
    /// request and service response.
    class IGNITION_VISIBLE AdvertiseSrv : public AdvertiseBase
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
                           const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_nUuid,
                           const Scope &_scope,
                           const std::string &_reqTypeName,
                           const std::string &_repTypeName);

      /// \brief Get the name of the request's protobuf message advertised.
      /// \return The protobuf message type.
      public: std::string ReqTypeName() const;

      /// \brief Get the name of the response's protobuf message advertised.
      /// \return The protobuf message type.
      public: std::string RepTypeName() const;

      /// \brief Set the name of the request's protobuf message advertised.
      /// \param[in] The protobuf message type.
      public: void ReqTypeName(const std::string &_reqTypeName);

      /// \brief Set the name of the response's protobuf message advertised.
      /// \param[in] The protobuf message type.
      public: void RepTypeName(const std::string &_repTypeName);

      // Documentation inherited.
      public: size_t MsgLength();

      // Documentation inherited.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const AdvertiseSrv &_msg)
      {
         _out << static_cast<const AdvertiseBase&>(_msg)
              << "\tRequest type: " << _msg.GetReqTypeName() << std::endl
              << "\tResponse type: " << _msg.GetRepTypeName() << std::endl;

        return _out;
      }

      // Documentation inherited.
      public: size_t Pack(char *_buffer);

      // Documentation inherited.
      public: size_t UnpackBody(char *_buffer);

      /// \brief The name of the request's protobuf message advertised.
      private: std::string reqTypeName = "";

      /// \brief The name of the response's protobuf message advertised.
      private: std::string repTypeName = "";
    };
  }
}

#endif
