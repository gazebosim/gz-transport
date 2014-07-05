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

#include <cstring>
#include <string>
#include "ignition/transport/Packet.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Header::Header(const uint16_t _version,
               const std::string &_pUuid,
               const uint8_t _type,
               const uint16_t _flags)
{
  this->SetVersion(_version);
  this->SetPUuid(_pUuid);
  this->SetType(_type);
  this->SetFlags(_flags);
}

//////////////////////////////////////////////////
uint16_t Header::GetVersion() const
{
  return this->version;
}

//////////////////////////////////////////////////
std::string Header::GetPUuid() const
{
  return this->pUuid;
}

//////////////////////////////////////////////////
uint8_t Header::GetType() const
{
  return this->type;
}

//////////////////////////////////////////////////
uint16_t Header::GetFlags() const
{
  return this->flags;
}

//////////////////////////////////////////////////
void Header::SetVersion(const uint16_t _version)
{
  this->version = _version;
}

//////////////////////////////////////////////////
void Header::SetPUuid(const std::string &_pUuid)
{
  this->pUuid = _pUuid;
}

//////////////////////////////////////////////////
void Header::SetType(const uint8_t _type)
{
  this->type = _type;
}

//////////////////////////////////////////////////
void Header::SetFlags(const uint16_t _flags)
{
  this->flags = _flags;
}

//////////////////////////////////////////////////
int Header::GetHeaderLength()
{
  return sizeof(this->version) +
         sizeof(this->pUuid.size()) + this->pUuid.size() +
         sizeof(this->type) + sizeof(this->flags);
}

//////////////////////////////////////////////////
size_t Header::Pack(char *_buffer)
{
  // Uninitialized.
  if ((this->version == 0) || (this->pUuid == "") ||
      (this->type  == Uninitialized))
  {
    std::cerr << "Header::Pack() error: You're trying to pack an incomplete "
              << "header:" << std::endl << *this;
    return 0;
  }

  memcpy(_buffer, &this->version, sizeof(this->version));
  _buffer += sizeof(this->version);
  size_t pUuidLength = this->pUuid.size();
  memcpy(_buffer, &pUuidLength, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);
  memcpy(_buffer, this->pUuid.data(), pUuidLength);
  _buffer += pUuidLength;
  memcpy(_buffer, &this->type, sizeof(this->type));
  _buffer += sizeof(this->type);
  memcpy(_buffer, &this->flags, sizeof(this->flags));

  return this->GetHeaderLength();
}

//////////////////////////////////////////////////
size_t Header::Unpack(const char *_buffer)
{
  // Read the version.
  memcpy(&this->version, _buffer, sizeof(this->version));
  _buffer += sizeof(this->version);

  // Read the process UUID length.
  size_t pUuidLength;
  memcpy(&pUuidLength, _buffer, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Read the process UUID.
  this->pUuid = std::string(_buffer, _buffer + pUuidLength);
  _buffer += pUuidLength;

  // Read the message type.
  memcpy(&this->type, _buffer, sizeof(this->type));
  _buffer += sizeof(this->type);

  // Read the flags.
  memcpy(&this->flags, _buffer, sizeof(this->flags));
  _buffer += sizeof(this->flags);

  return this->GetHeaderLength();
}

//////////////////////////////////////////////////
Sub::Sub(const Header &_header,
         const std::string &_topic)
{
  this->SetHeader(_header);
  this->SetTopic(_topic);
}

//////////////////////////////////////////////////
Header Sub::GetHeader() const
{
  return this->header;
}

//////////////////////////////////////////////////
std::string Sub::GetTopic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
void Sub::SetHeader(const Header &_header)
{
  this->header = _header;
}

//////////////////////////////////////////////////
void Sub::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
size_t Sub::GetMsgLength()
{
  return this->header.GetHeaderLength() +
         sizeof(this->topic.size()) + this->topic.size();
}

//////////////////////////////////////////////////
size_t Sub::Pack(char *_buffer)
{
  size_t headerLen = this->GetHeader().Pack(_buffer);
  if (headerLen == 0)
    return 0;

  if (this->topic == "")
  {
    std::cerr << "SubMsg::Pack() error: You're trying to pack a message with "
              << "an empty topic" << std::endl;
    return 0;
  }

  _buffer += headerLen;

  size_t topicLength = this->topic.size();
  memcpy(_buffer, &topicLength, sizeof(topicLength));
  _buffer += sizeof(topicLength);
  memcpy(_buffer, this->topic.data(), topicLength);

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t Sub::UnpackBody(char *_buffer)
{
  // Read the topic length.
  size_t topicLength;
  memcpy(&topicLength, _buffer, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Read the topic.
  this->topic = std::string(_buffer, _buffer + topicLength);
  _buffer += topicLength;

  return sizeof(topicLength) + topicLength;
}

//////////////////////////////////////////////////
Adv::Adv(const Header &_header,
         const std::string &_topic,
         const std::string &_addr,
         const std::string &_ctrl,
         const std::string &_nUuid,
         const Scope &_scope)
{
  this->SetHeader(_header);
  this->SetTopic(_topic);
  this->SetAddress(_addr);
  this->SetControlAddress(_ctrl);
  this->SetNodeUuid(_nUuid);
  this->SetScope(_scope);
}

//////////////////////////////////////////////////
Header Adv::GetHeader() const
{
  return this->header;
}

//////////////////////////////////////////////////
std::string Adv::GetTopic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
std::string Adv::GetAddress() const
{
  return this->addr;
}

//////////////////////////////////////////////////
std::string Adv::GetControlAddress() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
std::string Adv::GetNodeUuid() const
{
  return this->nUuid;
}

//////////////////////////////////////////////////
transport::Scope Adv::GetScope() const
{
  return this->scope;
}

//////////////////////////////////////////////////
void Adv::SetHeader(const Header &_header)
{
  this->header = _header;
}

//////////////////////////////////////////////////
void Adv::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
void Adv::SetAddress(const std::string &_addr)
{
  this->addr = _addr;
}

//////////////////////////////////////////////////
void Adv::SetControlAddress(const std::string &_ctrl)
{
  this->ctrl = _ctrl;
}

//////////////////////////////////////////////////
void Adv::SetNodeUuid(const std::string &_nUuid)
{
  this->nUuid = _nUuid;
}

//////////////////////////////////////////////////
void Adv::SetScope(const Scope &_scope)
{
  this->scope = _scope;
}

//////////////////////////////////////////////////
size_t Adv::GetMsgLength()
{
  return this->header.GetHeaderLength() +
         sizeof(this->topic.size()) + this->topic.size() +
         sizeof(this->addr.size()) + this->addr.size() +
         sizeof(this->ctrl.size()) + this->ctrl.size() +
         sizeof(this->nUuid.size()) + this->nUuid.size() +
         sizeof(this->scope);
}

//////////////////////////////////////////////////
size_t Adv::Pack(char *_buffer)
{
  size_t headerLen = this->GetHeader().Pack(_buffer);
  if (headerLen == 0)
    return 0;

  if ((this->topic == "") || (this->addr == "") || (this->nUuid == ""))
  {
    std::cerr << "Adv::Pack() error: You're trying to pack an incomplete "
              << "msg body:" << std::endl << *this;
    return 0;
  }

  _buffer += headerLen;

  size_t topicLength = this->topic.size();
  memcpy(_buffer, &topicLength, sizeof(topicLength));
  _buffer += sizeof(topicLength);
  memcpy(_buffer, this->topic.data(), topicLength);
  _buffer += topicLength;
  size_t addrLength = this->addr.size();
  memcpy(_buffer, &addrLength, sizeof(addrLength));
  _buffer += sizeof(addrLength);
  memcpy(_buffer, this->addr.data(), addrLength);
  _buffer += addrLength;
  size_t ctrlLength = this->ctrl.size();
  memcpy(_buffer, &ctrlLength, sizeof(ctrlLength));
  _buffer += sizeof(ctrlLength);
  memcpy(_buffer, this->ctrl.data(), ctrlLength);
  _buffer += ctrlLength;
  size_t nUuidLength = this->nUuid.size();
  memcpy(_buffer, &nUuidLength, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);
  memcpy(_buffer, this->nUuid.data(), nUuidLength);
  _buffer += nUuidLength;
  memcpy(_buffer, &this->scope, sizeof(this->scope));

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t Adv::UnpackBody(char *_buffer)
{
  // Read the topic length.
  size_t topicLength;
  memcpy(&topicLength, _buffer, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Read the topic.
  this->topic = std::string(_buffer, _buffer + topicLength);
  _buffer += topicLength;

  // Read the address length.
  size_t addrLength;
  memcpy(&addrLength, _buffer, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Read the address.
  this->addr = std::string(_buffer, _buffer + addrLength);
  _buffer += addrLength;

  // Read the control address length.
  size_t ctrlLength;
  memcpy(&ctrlLength, _buffer, sizeof(ctrlLength));
  _buffer += sizeof(ctrlLength);

  // Read the control address.
  this->ctrl = std::string(_buffer, _buffer + ctrlLength);
  _buffer += ctrlLength;

  // Read the node UUID length.
  size_t nUuidLength;
  memcpy(&nUuidLength, _buffer, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Read the node UUID.
  this->nUuid = std::string(_buffer, _buffer + nUuidLength);
  _buffer += nUuidLength;

  // Read the topic scope.
  memcpy(&this->scope, _buffer, sizeof(this->scope));

  return sizeof(topicLength) + topicLength +
         sizeof(addrLength) + addrLength +
         sizeof(ctrlLength) + ctrlLength +
         sizeof(nUuidLength) + nUuidLength +
         sizeof(this->scope);
}

//////////////////////////////////////////////////
AdvMsg::AdvMsg(const Header &_header,
               const std::string &_topic,
               const std::string &_addr,
               const std::string &_ctrl,
               const std::string &_nUuid,
               const Scope &_scope,
               const std::string &_msgTypeName)
  : Adv(_header, _topic, _addr, _ctrl, _nUuid, _scope)
{
  this->SetMsgTypeName(_msgTypeName);
}

//////////////////////////////////////////////////
std::string AdvMsg::GetMsgTypeName() const
{
  return this->msgTypeName;
}

//////////////////////////////////////////////////
void AdvMsg::SetMsgTypeName(const std::string &_msgTypeName)
{
  this->msgTypeName = _msgTypeName;
}

//////////////////////////////////////////////////
size_t AdvMsg::GetMsgLength()
{
  return Adv::GetMsgLength() +
         sizeof(this->msgTypeName.size()) + this->msgTypeName.size();
}

//////////////////////////////////////////////////
size_t AdvMsg::Pack(char *_buffer)
{
  size_t len = Adv::Pack(_buffer);
  if (len == 0)
    return 0;

  if (this->msgTypeName == "")
  {
    std::cerr << "AdvMsg::Pack() error: You're trying to pack a message with "
              << "an empty msgTypeName" << std::endl;
    return 0;
  }

  _buffer += len;

  size_t msgTypeNameLength = this->msgTypeName.size();
  memcpy(_buffer, &msgTypeNameLength, sizeof(msgTypeNameLength));
  _buffer += sizeof(msgTypeNameLength);
  memcpy(_buffer, this->msgTypeName.data(), msgTypeNameLength);

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t AdvMsg::UnpackBody(char *_buffer)
{
  _buffer += Adv::UnpackBody(_buffer);

  // Read the msgTypeName length.
  size_t msgTypeNameLen;
  memcpy(&msgTypeNameLen, _buffer, sizeof(msgTypeNameLen));
  _buffer += sizeof(msgTypeNameLen);

  // Read the msgTypeName.
  this->msgTypeName = std::string(_buffer, _buffer + msgTypeNameLen);

  return this->GetMsgLength() - this->GetHeader().GetHeaderLength();
}

//////////////////////////////////////////////////
AdvSrv::AdvSrv(const Header &_header,
               const std::string &_topic,
               const std::string &_addr,
               const std::string &_ctrl,
               const std::string &_nUuid,
               const Scope &_scope,
               const std::string &_reqTypeName,
               const std::string &_repTypeName)
  : Adv(_header, _topic, _addr, _ctrl, _nUuid, _scope)
{
  this->SetReqTypeName(_reqTypeName);
  this->SetRepTypeName(_repTypeName);
}

//////////////////////////////////////////////////
std::string AdvSrv::GetReqTypeName() const
{
  return this->reqTypeName;
}

//////////////////////////////////////////////////
std::string AdvSrv::GetRepTypeName() const
{
  return this->repTypeName;
}

//////////////////////////////////////////////////
void AdvSrv::SetReqTypeName(const std::string &_reqTypeName)
{
  this->reqTypeName = _reqTypeName;
}

//////////////////////////////////////////////////
void AdvSrv::SetRepTypeName(const std::string &_repTypeName)
{
  this->repTypeName = _repTypeName;
}

//////////////////////////////////////////////////
size_t AdvSrv::GetMsgLength()
{
  return Adv::GetMsgLength() +
         sizeof(this->reqTypeName.size()) + this->reqTypeName.size() +
         sizeof(this->repTypeName.size()) + this->repTypeName.size();
}

//////////////////////////////////////////////////
size_t AdvSrv::Pack(char *_buffer)
{
  size_t len = Adv::Pack(_buffer);
  if (len == 0)
    return 0;

  if ((this->reqTypeName == "") || (this->repTypeName == ""))
  {
    std::cerr << "AdvSrv::Pack() error: You're trying to pack an incomplete "
              << "msg body:" << std::endl << *this << std::endl;
    return 0;
  }

  _buffer += len;

  size_t reqTypeNameLength = this->reqTypeName.size();
  memcpy(_buffer, &reqTypeNameLength, sizeof(reqTypeNameLength));
  _buffer += sizeof(reqTypeNameLength);
  memcpy(_buffer, this->reqTypeName.data(), reqTypeNameLength);
  _buffer += reqTypeNameLength;
  size_t repTypeNameLength = this->repTypeName.size();
  memcpy(_buffer, &repTypeNameLength, sizeof(repTypeNameLength));
  _buffer += sizeof(repTypeNameLength);
  memcpy(_buffer, this->repTypeName.data(), repTypeNameLength);

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t AdvSrv::UnpackBody(char *_buffer)
{
  _buffer += Adv::UnpackBody(_buffer);

  // Read the reqTypeName length.
  size_t reqTypeNameLen;
  memcpy(&reqTypeNameLen, _buffer, sizeof(reqTypeNameLen));
  _buffer += sizeof(reqTypeNameLen);

  // Read the reqTypeName.
  this->reqTypeName = std::string(_buffer, _buffer + reqTypeNameLen);
  _buffer += reqTypeNameLen;

  // Read the repTypeName length.
  size_t repTypeNameLen;
  memcpy(&repTypeNameLen, _buffer, sizeof(repTypeNameLen));
  _buffer += sizeof(repTypeNameLen);

  // Read the repTypeName.
  this->repTypeName = std::string(_buffer, _buffer + repTypeNameLen);

  return this->GetMsgLength() - this->GetHeader().GetHeaderLength();
}
