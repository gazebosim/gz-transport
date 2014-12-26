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
    /// the scope of a topic:
    /// * Process: Topic only available to subscribers in the same process as
    ///            the publisher.
    /// * Host:    Topic only available to subscribers in the same machine as
    ///            the publisher.
    /// * All:     Topic available to any subscriber. This is the default scope.
    enum class Scope_t {Process, Host, All};

    /// \class MessagePublisher MessagePublisher.hh
    /// ignition/transport/MessagePublisher.hh
    /// \brief This class stores all the information about a message publisher.
    /// It stores the topic name that publishes, addresses, scope, etc.
    class IGNITION_VISIBLE Publisher
    {
      public: Publisher() = default;

      public: Publisher(const std::string &_topic,
                        const std::string &_addr,
                        const std::string &_pUuid,
                        const std::string &_nUuid,
                        const Scope_t _scope);

      public: virtual ~Publisher() = default;

      public: std::string Topic() const;

      public: std::string Addr() const;

      public: std::string PUuid() const;

      public: std::string NUuid() const;

      public: Scope_t Scope() const;

      public: void Topic(const std::string &_topic);

      public: void Addr(const std::string &_addr);

      public: void PUuid(const std::string &_pUuid);

      public: void NUuid(const std::string &_nUuid);

      public: void Scope(const Scope_t _scope);

      public: size_t Pack(char *_buffer);

      public: size_t Unpack(char *_buffer);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t GetMsgLength();

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

        return _out;
      }

      protected: std::string topic;

      protected: std::string addr;

      protected: std::string pUuid;

      protected: std::string nUuid;

      protected: Scope_t scope;
    };

    class IGNITION_VISIBLE MessagePublisher : public Publisher
    {
      public: MessagePublisher() = default;

      public: MessagePublisher(const std::string &_topic,
                               const std::string &_addr,
                               const std::string &_ctrl,
                               const std::string &_pUuid,
                               const std::string &_nUuid,
                               const Scope_t _scope,
                               const std::string &_msgTypeName);

      public: virtual ~MessagePublisher() = default;

      public: size_t Pack(char *_buffer);

      public: size_t Unpack(char *_buffer);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t GetMsgLength();

      public: std::string Ctrl() const;

      public: void Ctrl(const std::string &_ctrl);

      public: std::string MsgTypeName() const;

      public: void MsgTypeName(const std::string &_msgTypeName);

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg MessagePublisher to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const MessagePublisher &_msg)
      {
        _out << static_cast<Publisher>(_msg)
             << "\tControl address: " << _msg.Ctrl() << std::endl
             << "\tMessage type: " << _msg.MsgTypeName() << std::endl;
        return _out;
      }

      protected: std::string ctrl;

      protected: std::string msgTypeName;
    };

    class IGNITION_VISIBLE ServicePublisher : public Publisher
    {
      public: ServicePublisher() = default;

      public: ServicePublisher(const std::string &_topic,
                               const std::string &_addr,
                               const std::string &_id,
                               const std::string &_pUuid,
                               const std::string &_nUuid,
                               const Scope_t _scope,
                               const std::string &_reqType,
                               const std::string &_repType);

      public: virtual ~ServicePublisher() = default;

      public: size_t Pack(char *_buffer);

      public: size_t Unpack(char *_buffer);

      /// \brief Get the total length of the message.
      /// \return Return the length of the message in bytes.
      public: size_t GetMsgLength();

      public: std::string SocketId() const;

      public: void SocketId(const std::string &_socketId);

      /// \brief Get the name of the request's protobuf message advertised.
      /// \return The protobuf message type.
      public: std::string GetReqTypeName() const;

      /// \brief Get the name of the response's protobuf message advertised.
      /// \return The protobuf message type.
      public: std::string GetRepTypeName() const;

      /// \brief Set the name of the request's protobuf message advertised.
      /// \param[in] The protobuf message type.
      public: void SetReqTypeName(const std::string &_reqTypeName);

      /// \brief Set the name of the response's protobuf message advertised.
      /// \param[in] The protobuf message type.
      public: void SetRepTypeName(const std::string &_repTypeName);

      /// \brief Stream insertion operator.
      /// \param[out] _out The output stream.
      /// \param[in] _msg ServicePublisher to write to the stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                              const ServicePublisher &_msg)
      {
        _out << static_cast<Publisher>(_msg)
             << "\tSocket ID: " << _msg.SocketId() << std::endl
             << "\tRequest type: " << _msg.GetReqTypeName() << std::endl
             << "\tResponse type: " << _msg.GetRepTypeName() << std::endl;

        return _out;
      }

      protected: std::string socketId = "";

       /// \brief The name of the request's protobuf message advertised.
      private: std::string reqTypeName = "";

      /// \brief The name of the response's protobuf message advertised.
      private: std::string repTypeName = "";
    };
  }
}

#endif
