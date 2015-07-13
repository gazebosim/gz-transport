/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifndef __IGN_TRANSPORT_MESSAGEPUBLISHER_HH_INCLUDED__
#define __IGN_TRANSPORT_MESSAGEPUBLISHER_HH_INCLUDED__

#include <iostream>
#include <string>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    /// \def Scope This strongly typed enum defines the different options for
    /// the scope of a topic/service:
    /// * Process: Topic/service only available to subscribers in the same
    ///            process as the publisher.
    /// * Host:    Topic/service only available to subscribers in the same
    ///            machine as the publisher.
    /// * All:     Topic/service available to any subscriber (default scope).
    enum class Scope_t {Process, Host, All};

    /// \class Publisher Publisher.hh
    /// ignition/transport/Publisher.hh
    /// \brief This class stores all the information about a publisher.
    /// It stores the topic name that publishes, addresses, UUIDs, scope, etc.
    class IGNITION_VISIBLE Publisher
    {
      /// \brief Default constructor.
      public: Publisher() = default;

      /// \brief Constructor.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address.
      /// \param[in] _pUuid Process UUID.
      /// \param[in] _nUUID node UUID.
      /// \param[in] _scope Scope.
      public: Publisher(const std::string &_topic,
                        const std::string &_addr,
                        const std::string &_pUuid,
                        const std::string &_nUuid,
                        const Scope_t &_scope);

      /// \brief MessagePublisher Constructor.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address.
      /// \param[in] _ctrl ZeroMQ control address.
      /// \param[in] _pUuid Process UUID.
      /// \param[in] _nUUID node UUID.
      /// \param[in] _scope Scope.
      /// \param[in] _msgTypeName Message type advertised by this publisher.
      public: Publisher(const std::string &_topic,
                        const std::string &_addr,
                        const std::string &_ctrl,
                        const std::string &_pUuid,
                        const std::string &_nUuid,
                        const Scope_t &_scope,
                        const std::string &_msgTypeName);

      /// \brief Service Constructor.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr ZeroMQ address.
      /// \param[in] _id ZeroMQ socket ID.
      /// \param[in] _pUuid Process UUID.
      /// \param[in] _nUUID node UUID.
      /// \param[in] _scope Scope.
      /// \param[in] _reqType Message type used in the service request.
      /// \param[in] _repType Message type used in the service response.
      public: Publisher(const std::string &_topic,
                        const std::string &_addr,
                        const std::string &_id,
                        const std::string &_pUuid,
                        const std::string &_nUuid,
                        const Scope_t &_scope,
                        const std::string &_reqType,
                        const std::string &_repType);

      /// \brief Destructor.
      public: ~Publisher() = default;

      /// \brief Get the topic published by this publisher.
      /// \return Topic name.
      public: std::string Topic() const;

      /// \brief Get the ZeroMQ address of the publisher.
      /// \return ZeroMQ address.
      public: std::string Addr() const;

      /// \brief Get the process UUID of the publisher.
      /// return Process UUID.
      public: std::string PUuid() const;

      /// \brief Get the node UUID of the publisher.
      /// \return Node UUID.
      public: std::string NUuid() const;

      /// \brief Get the scope of the publisher's topic.
      /// \return Scope of the topic advertised by the publisher.
      public: Scope_t Scope() const;

      /// \brief Set the topic name published by this publisher.
      public: void Topic(const std::string &_topic);

      /// \brief Set ZeroMQ address of the publisher.
      public: void Addr(const std::string &_addr);

      /// \brief Set the process UUID of the publisher.
      public: void PUuid(const std::string &_pUuid);

      /// \brief Set the node UUID of the publisher.
      public: void NUuid(const std::string &_nUuid);

      /// \brief Set the scope of the topic advertised by this publisher.
      public: void Scope(const Scope_t &_scope);

      /// \brief Serialize the publisher. The caller has ownership of the
      /// buffer and is responsible for its [de]allocation.
      /// \param[out] _buffer Destination buffer in which the publisher
      /// will be serialized.
      /// \return Number of bytes serialized.
      public: size_t Pack(char *_buffer) const;

      /// \brief Unserialize the publisher.
      /// \param[in] _buffer Input buffer with the data to be unserialized.
      public: size_t Unpack(char *_buffer);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t MsgLength() const;

      /// \brief Get the ZeroMQ control address. This address is used by the
      /// subscribers to notify the publisher about the new subscription.
      /// \return ZeroMQ control address of the publisher.
      public: std::string Ctrl() const;

      /// \brief Set the ZeroMQ control address of the publisher.
      public: void Ctrl(const std::string &_ctrl);

      /// \brief Get the message type advertised by this publisher.
      /// \return Message type.
      public: std::string MsgTypeName() const;

      /// \brief Set the message type advertised by this publisher.
      public: void MsgTypeName(const std::string &_msgTypeName);

      /// \brief Get the ZeroMQ socket ID used by this publisher.
      /// \return The socket ID.
      public: std::string SocketId() const;

      /// \brief Set the ZeroMQ socket ID for this publisher.
      public: void SocketId(const std::string &_socketId);

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

      /// \brief whether the publisher is Service publisher
      /// \return bool
      public: bool isServicePublisher() const;

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg Publisher to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const Publisher &_msg)
      {
        _out << "Publisher:" << std::endl
             << "\tTopic: [" << _msg.Topic() << "]" << std::endl
             << "\tAddress: " << _msg.Addr() << std::endl
             << "\tProcess UUID: " << _msg.PUuid() << std::endl
             << "\tNode UUID: " << _msg.NUuid() << std::endl
             << "\tTopic Scope: ";
        if (_msg.Scope() == Scope_t::Process)
          _out << "Process" << std::endl;
        else if (_msg.Scope() == Scope_t::Host)
          _out << "Host" << std::endl;
        else
          _out << "All" << std::endl;

        if (_msg.isServicePublisher())
          _out << "\tSocket ID: "     << _msg.SocketId()  << std::endl
             << "\tRequest type: "  << _msg.ReqTypeName() << std::endl
             << "\tResponse type: " << _msg.RepTypeName() << std::endl;
        else
          _out << "\tControl address: " << _msg.Ctrl()      << std::endl
             << "\tMessage type: "    << _msg.MsgTypeName() << std::endl;

        return _out;
      }

      /// \brief Topic name.
      protected: std::string topic;

      /// \brief ZeroMQ address of the publisher.
      protected: std::string addr;

      /// \brief Process UUID of the publisher.
      protected: std::string pUuid;

      /// \brief Node UUID of the publisher.
      protected: std::string nUuid;

      /// \brief Scope of the topic advertised by this publisher.
      protected: Scope_t scope = Scope_t::All;

      /// \brief ZeroMQ control address/socket ID of the publisher.
      protected: std::string ctrl;

      /// \brief Message/Service type advertised by this publisher.
      protected: std::string msgTypeName;

      /// \brief The name of the response's protobuf message advertised.
      protected: std::string repTypeName;

      /// \brief Bool whether this publisher is a Message or Service publisher.
      protected: bool isService;

      /// \brief Bool whether this publisher is real publisher.
      protected: bool isReal;
    };
  }
}

#endif
