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

#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include "ignition/transport/Packet.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Header::Header()
  : headerLength(0)
{
}

//////////////////////////////////////////////////
Header::Header(const uint16_t _version,
               const std::string &_pUuid,
               const std::string &_topic,
               const uint8_t _type,
               const uint16_t _flags)
{
  this->SetVersion(_version);
  this->SetPUuid(_pUuid);
  this->SetTopic(_topic);
  this->SetType(_type);
  this->SetFlags(_flags);
  this->UpdateHeaderLength();
}

//////////////////////////////////////////////////
uint16_t Header::GetVersion() const
{
  return this->version;
}

//////////////////////////////////////////////////
std::string& Header::GetPUuid()
{
  return this->pUuid;
}

//////////////////////////////////////////////////
uint16_t Header::GetTopicLength() const
{
  return this->topicLength;
}

//////////////////////////////////////////////////
std::string Header::GetTopic() const
{
  return this->topic;
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
  this->pUuidLength = this->pUuid.size();
  this->UpdateHeaderLength();
}

//////////////////////////////////////////////////
void Header::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
  this->topicLength = this->topic.size();
  this->UpdateHeaderLength();
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
  return this->headerLength;
}

//////////////////////////////////////////////////
void Header::Print()
{
  std::cout << "\t--------------------------------------\n";
  std::cout << "\tHeader:" << std::endl;
  std::cout << "\t\tVersion: " << this->GetVersion() << "\n";
  std::cout << "\t\tProcess UUID: " << this->GetPUuid() << "\n";
  std::cout << "\t\tTopic length: " << this->GetTopicLength() << "\n";
  std::cout << "\t\tTopic: [" << this->GetTopic() << "]\n";
  std::cout << "\t\tType: " << MsgTypesStr.at(this->GetType()) << "\n";
  std::cout << "\t\tFlags: " << this->GetFlags() << "\n";
}

//////////////////////////////////////////////////
size_t Header::Pack(char *_buffer)
{
  if (this->headerLength == 0)
    return 0;

  memcpy(_buffer, &this->version, sizeof(this->version));
  _buffer += sizeof(this->version);
  memcpy(_buffer, &this->pUuidLength, sizeof(this->pUuidLength));
  _buffer += sizeof(this->pUuidLength);
  memcpy(_buffer, this->pUuid.data(), this->pUuidLength);
  _buffer += this->pUuidLength;
  memcpy(_buffer, &this->topicLength, sizeof(this->topicLength));
  _buffer += sizeof(this->topicLength);
  memcpy(_buffer, this->topic.data(), this->topicLength);
  _buffer += this->topicLength;
  memcpy(_buffer, &this->type, sizeof(this->type));
  _buffer += sizeof(this->type);
  memcpy(_buffer, &this->flags, sizeof(this->flags));

  return this->headerLength;
}

//////////////////////////////////////////////////
size_t Header::Unpack(const char *_buffer)
{
  // Read the version.
  memcpy(&this->version, _buffer, sizeof(this->version));
  _buffer += sizeof(this->version);

  // Read the process UUID length.
  memcpy(&this->pUuidLength, _buffer, sizeof(this->pUuidLength));
  _buffer += sizeof(this->pUuidLength);

  // Read the process UUID.
  this->pUuid = std::string(_buffer, _buffer + this->pUuidLength);
  _buffer += this->pUuidLength;

  // Read the topic length.
  memcpy(&this->topicLength, _buffer, sizeof(this->topicLength));
  _buffer += sizeof(this->topicLength);

  // Read the topic.
  this->topic = std::string(_buffer, _buffer + this->topicLength);
  _buffer += this->topicLength;

  // Read the message type.
  memcpy(&this->type, _buffer, sizeof(this->type));
  _buffer += sizeof(this->type);

  // Read the flags.
  memcpy(&this->flags, _buffer, sizeof(this->flags));
  _buffer += sizeof(this->flags);

  this->UpdateHeaderLength();
  return this->GetHeaderLength();
}

//////////////////////////////////////////////////
void Header::UpdateHeaderLength()
{
  this->headerLength = sizeof(this->version) +
                       sizeof(this->pUuidLength) + this->pUuidLength +
                       sizeof(this->topicLength) + this->topicLength +
                       sizeof(this->type) + sizeof(this->flags);
}

//////////////////////////////////////////////////
AdvMsg::AdvMsg()
  :  msgLength(0)
{
}

//////////////////////////////////////////////////
AdvMsg::AdvMsg(const Header &_header,
               const std::string &_address,
               const std::string &_controlAddress,
               const std::string &_nodeUuid,
               const Scope &_scope)
{
  this->SetHeader(_header);
  this->SetAddress(_address);
  this->SetControlAddress(_controlAddress);
  this->SetNodeUuid(_nodeUuid);
  this->SetScope(_scope);
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
Header& AdvMsg::GetHeader()
{
  return this->header;
}

//////////////////////////////////////////////////
uint16_t AdvMsg::GetAddressLength() const
{
  return this->addressLength;
}

//////////////////////////////////////////////////
std::string AdvMsg::GetAddress() const
{
  return this->address;
}

//////////////////////////////////////////////////
uint16_t AdvMsg::GetControlAddressLength() const
{
  return this->controlAddressLength;
}

//////////////////////////////////////////////////
std::string AdvMsg::GetControlAddress() const
{
  return this->controlAddress;
}

//////////////////////////////////////////////////
uint16_t AdvMsg::GetNodeUuidLength() const
{
  return this->nodeUuidLength;
}

//////////////////////////////////////////////////
std::string AdvMsg::GetNodeUuid() const
{
  return this->nodeUuid;
}

//////////////////////////////////////////////////
transport::Scope AdvMsg::GetScope() const
{
  return this->scope;
}

//////////////////////////////////////////////////
void AdvMsg::SetHeader(const Header &_header)
{
  this->header = _header;
  /*if (_header.GetType() != AdvType &&_header.GetType() != AdvSvcType)
  {
    std::cerr << "You're trying to use a "
              << MsgTypesStr.at(_header.GetType()) << " header inside an AdvMsg"
              << " or AdvSvcMsg. Are you sure you want to do this?\n";
  }*/
}

//////////////////////////////////////////////////
void AdvMsg::SetAddress(const std::string &_address)
{
  this->address = _address;
  this->addressLength = this->address.size();
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
void AdvMsg::SetControlAddress(const std::string &_address)
{
  this->controlAddress = _address;
  this->controlAddressLength = this->controlAddress.size();
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
void AdvMsg::SetNodeUuid(const std::string &_nUuid)
{
  this->nodeUuid = _nUuid;
  this->nodeUuidLength = this->nodeUuid.size();
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
void AdvMsg::SetScope(const Scope &_scope)
{
  this->scope = _scope;
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
size_t AdvMsg::GetMsgLength()
{
  return this->header.GetHeaderLength() +
         sizeof(this->addressLength) + this->addressLength +
         sizeof(this->controlAddressLength) + this->controlAddressLength +
         sizeof(this->nodeUuidLength) + this->nodeUuidLength +
         sizeof(this->scope);
}

//////////////////////////////////////////////////
void AdvMsg::PrintBody()
{
  std::cout << "\tBody:" << std::endl;
  std::cout << "\t\tAddr size: " << this->GetAddressLength() << std::endl;
  std::cout << "\t\tAddress: " << this->GetAddress() << std::endl;
  std::cout << "\t\tControl addr size: "
            << this->GetControlAddressLength() << std::endl;
  std::cout << "\t\tControl address: "
            << this->GetControlAddress() << std::endl;
  std::cout << "\t\tNode UUID: "
            << this->GetNodeUuid() << std::endl;
  std::cout << "\t\tTopic Scope: ";
  switch (this->GetScope())
  {
    case Scope::Process:
      std::cout << "Process" << std::endl;
      break;
    case Scope::Host:
      std::cout << "Host" << std::endl;
      break;
    case Scope::All:
      std::cout << "All" << std::endl;
      break;
    default:
      std::cout << "<Unknown>" << std::endl;
      break;
  }
}

//////////////////////////////////////////////////
size_t AdvMsg::Pack(char *_buffer)
{
  if (this->msgLength == 0)
  return 0;

  this->GetHeader().Pack(_buffer);
  _buffer += this->GetHeader().GetHeaderLength();

  memcpy(_buffer, &this->addressLength, sizeof(this->addressLength));
  _buffer += sizeof(this->addressLength);
  memcpy(_buffer, this->address.data(), this->addressLength);
  _buffer += this->addressLength;
  memcpy(_buffer, &this->controlAddressLength,
         sizeof(this->controlAddressLength));
  _buffer += sizeof(this->controlAddressLength);
  memcpy(_buffer, this->controlAddress.data(), this->controlAddressLength);
  _buffer += this->controlAddressLength;
  memcpy(_buffer, &this->nodeUuidLength, sizeof(this->nodeUuidLength));
  _buffer += sizeof(this->nodeUuidLength);
  memcpy(_buffer, this->nodeUuid.data(), this->nodeUuidLength);
  _buffer += this->nodeUuidLength;
  memcpy(_buffer, &this->scope, sizeof(this->scope));

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t AdvMsg::UnpackBody(char *_buffer)
{
  // Read the address length.
  memcpy(&this->addressLength, _buffer, sizeof(this->addressLength));
  _buffer += sizeof(this->addressLength);

  // Read the address.
  this->address = std::string(_buffer, _buffer + this->addressLength);
  _buffer += this->addressLength;

  // Read the control address length.
  memcpy(&this->controlAddressLength, _buffer,
    sizeof(this->controlAddressLength));
  _buffer += sizeof(this->controlAddressLength);

  // Read the control address.
  this->controlAddress =
    std::string(_buffer, _buffer + this->controlAddressLength);
  _buffer += this->controlAddressLength;

  // Read the node UUID length.
  memcpy(&this->nodeUuidLength, _buffer, sizeof(this->nodeUuidLength));
  _buffer += sizeof(this->nodeUuidLength);

  // Read the node UUID.
  this->nodeUuid = std::string(_buffer, _buffer + this->nodeUuidLength);
  _buffer += this->nodeUuidLength;

  // Read the topic scope.
  memcpy(&this->scope, _buffer, sizeof(this->scope));

  this->UpdateMsgLength();

  return sizeof(this->addressLength) + this->addressLength +
         sizeof(this->controlAddressLength) + this->controlAddressLength +
         sizeof(this->nodeUuidLength) + this->nodeUuidLength +
         sizeof(this->scope);
}

//////////////////////////////////////////////////
void AdvMsg::UpdateMsgLength()
{
  this->msgLength = this->GetHeader().GetHeaderLength() +
    sizeof(this->addressLength) + this->addressLength +
    sizeof(this->controlAddressLength) + this->controlAddressLength +
    sizeof(this->nodeUuidLength) + this->nodeUuidLength +
    sizeof(this->scope);
}
