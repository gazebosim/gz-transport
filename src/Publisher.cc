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
    scope(_scope),
    isService(false),
    isReal(false)
{
  auto topic_name = this->topic;
  topic_name.erase(0, topic_name.find_first_of("@")+1);

  auto first = topic_name.find_first_of("@");
  auto last = topic_name.find_first_of("@", first+1);
  if (last == std::string::npos)
  {
    last = first;
    first = -1;
  }
  auto type = topic_name.substr(first+1, last-first-1);

  this->isService = false;

  if (type == "srv")
    this->isService = true;
}

//////////////////////////////////////////////////
Publisher::Publisher(const std::string &_topic, const std::string &_addr,
  const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const Scope_t &_scope,
  const std::string &_msgTypeName)
  : topic(_topic),
    addr(_addr),
    pUuid(_pUuid),
    nUuid(_nUuid),
    scope(_scope),
    ctrl(_ctrl),
    msgTypeName(_msgTypeName),
    isService(false),
    isReal(true)
{
}

//////////////////////////////////////////////////
Publisher::Publisher(const std::string &_topic,
  const std::string &_addr, const std::string &_socketId,
  const std::string &_pUuid, const std::string &_nUuid, const Scope_t &_scope,
  const std::string &_reqType, const std::string &_repType)
  : topic(_topic),
    addr(_addr),
    pUuid(_pUuid),
    nUuid(_nUuid),
    scope(_scope),
    ctrl(_socketId),
    msgTypeName(_reqType),
    repTypeName(_repType),
    isService(true),
    isReal(true)
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
size_t Publisher::Pack(char *_buffer) const
{
  if (this->topic.empty() || this->addr.empty() ||
      this->pUuid.empty() || this->nUuid.empty())
  {
    std::cerr << "Publisher::Pack() error: You're trying to pack an "
              << "incomplete Publisher:" << std::endl << *this;
    return 0;
  }

  if (this->isReal && (this->ctrl.empty() || this->msgTypeName.empty() ||
          (this->isService && this->repTypeName.empty())))
  {
    std::cerr << "Publisher::Pack() error: You're trying to pack an "
              << "incomplete Publisher:" << std::endl << *this;
    return 0;
  }

  // null buffer.
  if (!_buffer)
  {
    std::cerr << "Publisher::Pack() error: NULL output buffer" << std::endl;
    return 0;
  }

  // Pack type of publisher
  memcpy(_buffer, &this->isReal, sizeof(this->isReal));
  _buffer += sizeof(this->isReal);

  memcpy(_buffer, &this->isService, sizeof(this->isService));
  _buffer += sizeof(this->isService);

  // Pack the topic length.
  uint64_t topicLength = this->topic.size();
  memcpy(_buffer, &topicLength, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Pack the topic.
  memcpy(_buffer, this->topic.data(), static_cast<size_t>(topicLength));
  _buffer += topicLength;

  // Pack the zeromq address length.
  uint64_t addrLength = this->addr.size();
  memcpy(_buffer, &addrLength, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Pack the zeromq address.
  memcpy(_buffer, this->addr.data(), static_cast<size_t>(addrLength));
  _buffer += addrLength;

  // Pack the process UUID length.
  uint64_t pUuidLength = this->pUuid.size();
  memcpy(_buffer, &pUuidLength, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Pack the process UUID.
  memcpy(_buffer, this->pUuid.data(), static_cast<size_t>(pUuidLength));
  _buffer += pUuidLength;

  // Pack the node UUID length.
  uint64_t nUuidLength = this->nUuid.size();
  memcpy(_buffer, &nUuidLength, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Pack the node UUID.
  memcpy(_buffer, this->nUuid.data(), static_cast<size_t>(nUuidLength));
  _buffer += nUuidLength;

  // Pack the topic scope.
  uint8_t intscope = static_cast<uint8_t>(this->scope);
  memcpy(_buffer, &intscope, sizeof(intscope));

  if (this->isReal)
  {
    _buffer += sizeof(intscope);
    // Pack the zeromq control address length.
    uint64_t ctrlLength = this->ctrl.size();
    memcpy(_buffer, &ctrlLength, sizeof(ctrlLength));
    _buffer += sizeof(ctrlLength);

    // Pack the zeromq control address.
    memcpy(_buffer, this->ctrl.data(), static_cast<size_t>(ctrlLength));
    _buffer += ctrlLength;

    // Pack the type name length.
    uint64_t typeNameLength = this->msgTypeName.size();
    memcpy(_buffer, &typeNameLength, sizeof(typeNameLength));
    _buffer += sizeof(typeNameLength);

    // Pack the type name.
    memcpy(_buffer, this->msgTypeName.data(),
      static_cast<size_t>(typeNameLength));

    if (this->isService)
    {
      _buffer += typeNameLength;
      // Pack the response type length.
      uint64_t repTypeLength = this->repTypeName.size();
      memcpy(_buffer, &repTypeLength, sizeof(repTypeLength));
      _buffer += sizeof(repTypeLength);

      // Pack the response.
      memcpy(_buffer, this->repTypeName.data(), static_cast<size_t>(repTypeLength));
    }
  }

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t Publisher::Unpack(char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "Publisher::Unpack() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  // Unpack type of publisher
  memcpy(&this->isReal, _buffer, sizeof(this->isReal));
  _buffer += sizeof(this->isReal);

  memcpy(&this->isService, _buffer, sizeof(this->isService));
  _buffer += sizeof(this->isService);

  // Unpack the topic length.
  uint64_t topicLength;
  memcpy(&topicLength, _buffer, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Unpack the topic.
  this->topic = std::string(_buffer, _buffer + topicLength);
  _buffer += topicLength;

  // Unpack the zeromq address length.
  uint64_t addrLength;
  memcpy(&addrLength, _buffer, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Unpack the zeromq address.
  this->addr = std::string(_buffer, _buffer + addrLength);
  _buffer += addrLength;

  // Unpack the process UUID length.
  uint64_t pUuidLength;
  memcpy(&pUuidLength, _buffer, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Unpack the process UUID.
  this->pUuid = std::string(_buffer, _buffer + pUuidLength);
  _buffer += pUuidLength;

  // Unpack the node UUID length.
  uint64_t nUuidLength;
  memcpy(&nUuidLength, _buffer, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Unpack the node UUID.
  this->nUuid = std::string(_buffer, _buffer + nUuidLength);
  _buffer += nUuidLength;

  // Unpack the topic scope.
  uint8_t intscope;
  memcpy(&intscope, _buffer, sizeof(intscope));
  this->scope = static_cast<Scope_t>(intscope);

  if (this->isReal)
  {
    _buffer += sizeof(intscope);
    // Unpack the zeromq control address length.
    uint64_t ctrlLength;
    memcpy(&ctrlLength, _buffer, sizeof(ctrlLength));
    _buffer += sizeof(ctrlLength);

    // Unpack the zeromq control address.
    this->ctrl = std::string(_buffer, _buffer + ctrlLength);
    _buffer += ctrlLength;

    // Unpack the type name length.
    uint64_t typeNameLength;
    memcpy(&typeNameLength, _buffer, sizeof(typeNameLength));
    _buffer += sizeof(typeNameLength);

    // Unpack the type name.
    this->msgTypeName = std::string(_buffer, _buffer + typeNameLength);

    if (this->isService)
    {
      _buffer += typeNameLength;
      // Unpack the response type length.
      uint64_t repTypeLength;
      memcpy(&repTypeLength, _buffer, sizeof(repTypeLength));
      _buffer += sizeof(repTypeLength);

      // Unpack the response type.
      this->repTypeName = std::string(_buffer, _buffer + repTypeLength);
    }
  }

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t Publisher::MsgLength() const
{
  auto size = sizeof(uint64_t) + this->topic.size() +
              sizeof(uint64_t) + this->addr.size() +
              sizeof(uint64_t) + this->pUuid.size() +
              sizeof(uint64_t) + this->nUuid.size() +
              sizeof(uint8_t);
  if (this->isReal)
  {
    if (this->isService)
      size += sizeof(uint64_t) + this->ctrl.size() +
              sizeof(uint64_t) + this->msgTypeName.size() +
              sizeof(uint64_t) + this->repTypeName.size();
    else
      size += sizeof(uint64_t) + this->ctrl.size() +
              sizeof(uint64_t) + this->msgTypeName.size();
  }

  return size;
}

//////////////////////////////////////////////////
std::string Publisher::Ctrl() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
void Publisher::Ctrl(const std::string &_ctrl)
{
  this->isReal = true;
  this->ctrl = _ctrl;
}

//////////////////////////////////////////////////
std::string Publisher::MsgTypeName() const
{
  return this->msgTypeName;
}

//////////////////////////////////////////////////
void Publisher::MsgTypeName(const std::string &_msgTypeName)
{
  this->isReal = true;
  this->msgTypeName = _msgTypeName;
}

//////////////////////////////////////////////////
std::string Publisher::SocketId() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
void Publisher::SocketId(const std::string &_socketId)
{
  this->isReal = true;
  this->ctrl = _socketId;
}

//////////////////////////////////////////////////
std::string Publisher::ReqTypeName() const
{
  return this->msgTypeName;
}

//////////////////////////////////////////////////
std::string Publisher::RepTypeName() const
{
  return this->repTypeName;
}

//////////////////////////////////////////////////
void Publisher::ReqTypeName(const std::string &_reqTypeName)
{
  this->isReal = true;
  this->msgTypeName = _reqTypeName;
}

//////////////////////////////////////////////////
void Publisher::RepTypeName(const std::string &_repTypeName)
{
  this->isReal = true;
  this->repTypeName = _repTypeName;
}

//////////////////////////////////////////////////
bool Publisher::isServicePublisher() const
{
  return this->isService;
}
