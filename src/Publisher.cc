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

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include "ignition/transport/Publisher.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Publisher::Publisher(const std::string &_topic, const std::string &_addr,
  const std::string &_pUuid, const std::string &_nUuid, const Scope_t &_scope)
  : topic(_topic),
    addr(_addr),
    pUuid(_pUuid),
    nUuid(_nUuid),
    scope(_scope)
{
}

//////////////////////////////////////////////////
std::string Publisher::Topic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
std::string Publisher::Addr() const
{
  return this->addr;
}

//////////////////////////////////////////////////
std::string Publisher::PUuid() const
{
  return this->pUuid;
}

//////////////////////////////////////////////////
std::string Publisher::NUuid() const
{
  return this->nUuid;
}

//////////////////////////////////////////////////
Scope_t Publisher::Scope() const
{
  return this->scope;
}

//////////////////////////////////////////////////
void Publisher::Topic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
void Publisher::Addr(const std::string &_addr)
{
  this->addr = _addr;
}

//////////////////////////////////////////////////
void Publisher::PUuid(const std::string &_pUuid)
{
  this->pUuid = _pUuid;
}

//////////////////////////////////////////////////
void Publisher::NUuid(const std::string &_nUuid)
{
  this->nUuid = _nUuid;
}

//////////////////////////////////////////////////
void Publisher::Scope(const Scope_t &_scope)
{
  this->scope = _scope;
}

//////////////////////////////////////////////////
bool Publisher::Pack(std::vector<char> &_buffer) const
{
  if ((this->topic == "") || (this->addr == "") ||
      (this->pUuid == "") || (this->nUuid == ""))
  {
    std::cerr << "Publisher::Pack() error: You're trying to pack an "
              << "incomplete Publisher:" << std::endl << *this;
    return false;
  }

  msgs::PublisherData publisher;
  publisher.set_topic(this->topic);
  publisher.set_addr(this->addr);
  publisher.set_puuid(this->pUuid);
  publisher.set_nuuid(this->nUuid);
  publisher.set_scope(static_cast<uint32_t>(this->scope));

  // Pack using protobuf messages.
  return serialize(publisher, _buffer);
}

//////////////////////////////////////////////////
bool Publisher::Unpack(const std::vector<char> &_buffer)
{
  // Empty buffer.
  if (_buffer.empty())
  {
    std::cerr << "Publisher::Unpack() error: Empty input buffer" << std::endl;
    return false;
  }

  msgs::PublisherData message;

  // Unpack using protobuf messages.
  if (!unserialize(_buffer, message))
    return false;

  this->topic = message.topic();
  this->addr  = message.addr();
  this->pUuid = message.puuid();
  this->nUuid = message.nuuid();
  this->scope = static_cast<Scope_t>(message.scope());

  return true;
}

//////////////////////////////////////////////////
MessagePublisher::MessagePublisher(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const Scope_t &_scope,
  const std::string &_msgTypeName)
  : Publisher(_topic, _addr, _pUuid, _nUuid, _scope),
    ctrl(_ctrl),
    msgTypeName(_msgTypeName)
{
}

//////////////////////////////////////////////////
std::string MessagePublisher::Ctrl() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
void MessagePublisher::Ctrl(const std::string &_ctrl)
{
  this->ctrl = _ctrl;
}

//////////////////////////////////////////////////
std::string MessagePublisher::MsgTypeName() const
{
  return this->msgTypeName;
}

//////////////////////////////////////////////////
void MessagePublisher::MsgTypeName(const std::string &_msgTypeName)
{
  this->msgTypeName = _msgTypeName;
}

//////////////////////////////////////////////////
bool MessagePublisher::Pack(std::vector<char> &_buffer) const
{
  if ((this->ctrl == "") || (this->msgTypeName == ""))
  {
    std::cerr << "MessagePublisher::Pack() error: You're trying to pack an "
              << "incomplete Publisher:" << std::endl << *this;
    return false;
  }

  if (!Publisher::Pack(_buffer))
    return false;

  msgs::PublisherData publisher;
  if (!unserialize(_buffer, publisher))
    return false;


  publisher.set_ctrl(this->ctrl);
  publisher.set_msgtypename(this->msgTypeName);

  return serialize(publisher, _buffer);
}

//////////////////////////////////////////////////
bool MessagePublisher::Unpack(const std::vector<char> &_buffer)
{
  // Empty buffer.
  if (_buffer.empty())
  {
    std::cerr << "MessagePublisher::Unpack() error: Empty buffer" << std::endl;
    return false;
  }

  if (!Publisher::Unpack(_buffer))
    return false;

  msgs::PublisherData publisher;
  if (!unserialize(_buffer, publisher))
    return false;

  this->ctrl = publisher.ctrl();
  this->msgTypeName = publisher.msgtypename();

  return true;
}

//////////////////////////////////////////////////
ServicePublisher::ServicePublisher(const std::string &_topic,
  const std::string &_addr, const std::string &_socketId,
  const std::string &_pUuid, const std::string &_nUuid, const Scope_t &_scope,
  const std::string &_reqType, const std::string &_repType)
  : Publisher(_topic, _addr, _pUuid, _nUuid, _scope),
    socketId(_socketId),
    reqTypeName(_reqType),
    repTypeName(_repType)
{
}

//////////////////////////////////////////////////
std::string ServicePublisher::SocketId() const
{
  return this->socketId;
}

//////////////////////////////////////////////////
void ServicePublisher::SocketId(const std::string &_socketId)
{
  this->socketId = _socketId;
}

//////////////////////////////////////////////////
std::string ServicePublisher::ReqTypeName() const
{
  return this->reqTypeName;
}

//////////////////////////////////////////////////
std::string ServicePublisher::RepTypeName() const
{
  return this->repTypeName;
}

//////////////////////////////////////////////////
void ServicePublisher::ReqTypeName(const std::string &_reqTypeName)
{
  this->reqTypeName = _reqTypeName;
}

//////////////////////////////////////////////////
void ServicePublisher::RepTypeName(const std::string &_repTypeName)
{
  this->repTypeName = _repTypeName;
}

//////////////////////////////////////////////////
bool ServicePublisher::Pack(std::vector<char> &_buffer) const
{
  if ((this->socketId == "") || (this->reqTypeName == "") ||
      (this->repTypeName == ""))
  {
    std::cerr << "ServicePublisher::Pack() error: You're trying to pack an "
              << "incomplete Publisher:" << std::endl << *this;
    return false;
  }

  if (!Publisher::Pack(_buffer))
    return false;

  msgs::PublisherData publisher;
  if (!unserialize(_buffer, publisher))
    return false;

  publisher.set_ctrl(this->socketId);
  publisher.set_msgtypename(this->reqTypeName);
  publisher.set_reptypename(this->repTypeName);

  return serialize(publisher, _buffer);
}

//////////////////////////////////////////////////
bool ServicePublisher::Unpack(const std::vector<char> &_buffer)
{
  // Empty buffer.
  if (_buffer.empty())
  {
    std::cerr << "ServicePublisher::Unpack() error: Empty buffer" << std::endl;
    return false;
  }

  if (!Publisher::Unpack(_buffer))
    return false;

  msgs::PublisherData publisher;
  if (!unserialize(_buffer, publisher))
    return false;

  this->socketId = publisher.ctrl();
  this->reqTypeName = publisher.msgtypename();
  this->repTypeName = publisher.reptypename();

  return true;
}
