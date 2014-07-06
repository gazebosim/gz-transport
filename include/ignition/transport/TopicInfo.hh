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

#ifndef __IGN_TRANSPORT_TOPICSTORAGE_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICSTORAGE_HH_INCLUDED__

#include <string>
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class TopicInfo TopicInfo.hh ignition/transport/TopicInfo.hh
    /// \brief Store address information about a topic.
    class IGNITION_VISIBLE TopicInfo
    {
      /// \brief Constructor.
      protected: TopicInfo() = default;

      /// \brief Constructor.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ address of the publisher advertising the topic.
      /// \param[in] _ctrl 0MQ control address of the publisher advertising the
      ///  topic.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[in] _scope Topic Scope.
      protected: TopicInfo(const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_pUuid,
                           const std::string &_nUuid,
                           const Scope &_scope = Scope::All);

      /// \brief Destructor.
      public: virtual ~TopicInfo() = default;

      /// \brief Get the topic name.
      /// \return Topic name.
      public: std::string GetTopic() const;

      /// \brief Get the 0MQ publisher's address.
      /// \return The publisher's address.
      public: std::string GetAddr() const;

      /// \brief Get the 0MQ publisher's control address.
      /// \return The publisher's control address.
      public: std::string GetCtrl() const;

      /// \brief Get the process UUID of the publisher.
      /// \return The process UUID.
      public: std::string GetPUuid() const;

      /// \brief Get the node UUID of the publisher.
      /// \return The node UUID.
      public: std::string GetNUuid() const;

      /// \brief Get the scope of the topic.
      /// \return The topic's scope.
      public: Scope GetScope() const;

      /// \brief Set the topic name.
      /// \param[in] _topic New topic name.
      public: void SetTopic(const std::string &_topic);

      /// \brief Set the 0MQ publisher's address.
      /// \param[in] _addr New publisher's address.
      public: void SetAddr(const std::string &_addr);

      /// \brief Set the 0MQ publisher's control address.
      /// \param[in] _ctrl New publisher's control address.
      public: void SetCtrl(const std::string &_ctrl);

      /// \brief Set the publisher's process UUID.
      /// \param[in] _pUuid New publisher's process UUID.
      public: void SetPUuid(const std::string &_pUuid);

      /// \brief Set the publisher's node UUID.
      /// \param[in] _nUuid New publisher's node UUID.
      public: void SetNUuid(const std::string &_nUuid);

      /// \brief Set the topic scope.
      /// \param[in] _scope New topic scope.
      public: void SetScope(const Scope _scope);

      /// \brief Topic name.
      protected: std::string topic;

      /// \brief 0MQ address of the publisher advertising the topic.
      protected: std::string addr;

      /// \brief 0MQ control address of the publisher advertising the topic.
      protected: std::string ctrl;

      /// \brief Process UUID of the publisher.
      protected: std::string pUuid;

      /// \brief Node UUID of the publisher.
      protected: std::string nUuid;

      /// \brief Topic Scope.
      protected: Scope scope;
    };

    /// \class MsgTopicInfo TopicInfo.hh ignition/transport/TopicInfo.hh
    /// \brief Store address information about a topic that belongs to a msg.
    class IGNITION_VISIBLE MsgTopicInfo : public TopicInfo
    {
      /// \brief Constructor.
      public: MsgTopicInfo() = default;

      /// \brief Constructor.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ address of the publisher advertising the topic.
      /// \param[in] _ctrl 0MQ control address of the publisher advertising the
      ///  topic.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[in] _msgType Protobuf message name advertised for the topic.
      /// \param[in] _msgHash Hash associated to the protobuf message definition
      /// \param[in] _scope Topic Scope.
      public: MsgTopicInfo(const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_pUuid,
                           const std::string &_nUuid,
                           const std::string &_msgType,
                           const size_t _msgHash,
                           const Scope &_scope = Scope::All);

      /// \brief Destructor.
      public: virtual ~MsgTopicInfo() = default;

      /// \brief Get the protobuf message name advertised for the topic.
      /// \return The message type.
      public: std::string GetMsgType() const;

      /// \brief Get the hash associated to the protobuf message definition.
      /// \return The message definition's hash.
      public: size_t GetMsgHash() const;

      /// \brief Set the protobuf type announced for this topic.
      /// \param[in] _msgType The protobuf type.
      public: void SetMsgType(const std::string &_msgType);

      /// \brief Set the hash associated to the protobuf message definition.
      /// \param[in] _msgHash The messages's hash.
      public: void SetMsgHash(const size_t _msgHash);

      /// \brief Protobuf type name.
      protected: std::string msgType;

      /// \brief Hash associated to the protobuf message definition.
      protected: size_t msgHash;
    };

    /// \class SrvTopicInfo TopicInfo.hh ignition/transport/TopicInfo.hh
    /// \brief Store address information about a topic that belongs to a service
    class IGNITION_VISIBLE SrvTopicInfo : public TopicInfo
    {
      /// \brief Constructor.
      public: SrvTopicInfo() = default;

      /// \brief Constructor.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ address of the publisher advertising the topic.
      /// \param[in] _ctrl 0MQ control address of the publisher advertising the
      ///  topic.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[in] _reqType Protobuf message name used in the service request.
      /// \param[in] _reqHash Hash associated to the protobuf's message request.
      /// \param[in] _repType Protobuf message name used in the service response
      /// \param[in] _repHash Hash associated to the protobuf's message response
      /// \param[in] _scope Topic Scope.
      public: SrvTopicInfo(const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_pUuid,
                           const std::string &_nUuid,
                           const std::string &_reqType,
                           const size_t _reqHash,
                           const std::string &_repType,
                           const size_t _repHash,
                           const Scope &_scope = Scope::All);

      /// \brief Destructor.
      public: virtual ~SrvTopicInfo() = default;

      /// \brief Get the protobuf message name used in the service request.
      /// \return The message type used in the service request.
      public: std::string GetReqType() const;

      /// \brief Get the hash associated to the protobuf's message request.
      /// \return The protobuf's message request type.
      public: size_t GetReqHash() const;

      /// \brief Get the protobuf message name used in the service response.
      /// \return The message type used in the service response.
      public: std::string GetRepType() const;

      /// \brief Get the hash associated to the protobuf's message response.
      /// \return The protobuf's message response type.
      public: size_t GetRepHash() const;

      /// \brief Set the protobuf type announced for the service request.
      /// \param[in] _reqType The protobuf type.
      public: void SetReqType(const std::string &_reqType);

      /// \brief Set the hash associated to the protobuf's message request.
      /// \param[in] _reqHash The messages's hash.
      public: void SetReqHash(const size_t _reqHash);

      /// \brief Set the protobuf type announced for the service response.
      /// \param[in] _repType The protobuf type.
      public: void SetRepType(const std::string &_repType);

      /// \brief Set the hash associated to the protobuf's message response.
      /// \param[in] _repHash The messages's hash.
      public: void SetRepHash(const size_t _repHash);

      /// \brief Protobuf type for the service request.
      protected: std::string reqType;

      /// \brief Hash associated to the protobuf's message request.
      protected: size_t reqHash;

      /// \brief Protobuf type for the service response.
      protected: std::string repType;

      /// \brief Hash associated to the protobuf's message response.
      protected: size_t repHash;
    };
  }
}

#endif
