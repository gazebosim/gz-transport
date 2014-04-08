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
#include <uuid/uuid.h>
#include <iostream>
#include <string>
#include "ignition/transport/packet.hh"

//////////////////////////////////////////////////
std::string transport::GetGuidStr(const uuid_t &_uuid)
{
  char *guid_str = new char[GUID_STR_LEN];
  for (size_t i = 0; i < sizeof(uuid_t) && i != GUID_STR_LEN; ++i)
  {
    snprintf(guid_str, GUID_STR_LEN,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      _uuid[0], _uuid[1], _uuid[2], _uuid[3],
      _uuid[4], _uuid[5], _uuid[6], _uuid[7],
      _uuid[8], _uuid[9], _uuid[10], _uuid[11],
      _uuid[12], _uuid[13], _uuid[14], _uuid[15]);
  }
  return std::string(guid_str);
}

//////////////////////////////////////////////////
transport::Header::Header()
  : headerLength(0)
{
}

//////////////////////////////////////////////////
transport::Header::Header(const uint16_t _version,
                          const uuid_t &_guid,
                          const std::string &_topic,
                          const uint8_t _type,
                          const uint16_t _flags)
{
  this->SetVersion(_version);
  this->SetGuid(_guid);
  this->SetTopic(_topic);
  this->SetType(_type);
  this->SetFlags(_flags);
  this->UpdateHeaderLength();
}

//////////////////////////////////////////////////
uint16_t transport::Header::GetVersion() const
{
  return this->version;
}

//////////////////////////////////////////////////
uuid_t& transport::Header::GetGuid()
{
  return this->guid;
}

//////////////////////////////////////////////////
uint16_t transport::Header::GetTopicLength() const
{
  return this->topicLength;
}

//////////////////////////////////////////////////
std::string transport::Header::GetTopic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
uint8_t transport::Header::GetType() const
{
  return this->type;
}

//////////////////////////////////////////////////
uint16_t transport::Header::GetFlags() const
{
  return this->flags;
}

//////////////////////////////////////////////////
void transport::Header::SetVersion(const uint16_t _version)
{
  this->version = _version;
}

//////////////////////////////////////////////////
void transport::Header::SetGuid(const uuid_t &_guid)
{
  uuid_copy(this->guid, _guid);
}

//////////////////////////////////////////////////
void transport::Header::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
  this->topicLength = this->topic.size();
  this->UpdateHeaderLength();
}

//////////////////////////////////////////////////
void transport::Header::SetType(const uint8_t _type)
{
  this->type = _type;
}

//////////////////////////////////////////////////
void transport::Header::SetFlags(const uint16_t _flags)
{
  this->flags = _flags;
}

//////////////////////////////////////////////////
int transport::Header::GetHeaderLength()
{
  return this->headerLength;
}

//////////////////////////////////////////////////
void transport::Header::Print()
{
  std::cout << "\t--------------------------------------\n";
  std::cout << "\tHeader:" << std::endl;
  std::cout << "\t\tVersion: " << this->GetVersion() << "\n";
  std::cout << "\t\tGUID: " << this->GetGuid() << "\n";
  std::cout << "\t\tTopic length: " << this->GetTopicLength() << "\n";
  std::cout << "\t\tTopic: [" << this->GetTopic() << "]\n";
  std::cout << "\t\tType: " << msgTypesStr[this->GetType()] << "\n";
  std::cout << "\t\tFlags: " << this->GetFlags() << "\n";
}

//////////////////////////////////////////////////
size_t transport::Header::Pack(char *_buffer)
{
  if (this->headerLength == 0)
    return 0;

  memcpy(_buffer, &this->version, sizeof(this->version));
  _buffer += sizeof(this->version);
  memcpy(_buffer, &this->guid, sizeof(this->guid));
  _buffer += sizeof(this->guid);
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
size_t transport::Header::Unpack(const char *_buffer)
{
  // Read the version
  memcpy(&this->version, _buffer, sizeof(this->version));
  _buffer += sizeof(this->version);

  // Read the GUID
  memcpy(&this->guid, _buffer, sizeof(this->guid));
  _buffer += sizeof(this->guid);

  // Read the topic length
  memcpy(&this->topicLength, _buffer, sizeof(this->topicLength));
  _buffer += sizeof(this->topicLength);

  // Read the topic
  char *newTopic = new char[this->topicLength + 1];
  memcpy(newTopic, _buffer, this->topicLength);
  newTopic[this->topicLength] = '\0';
  this->topic = newTopic;
  _buffer += this->topicLength;
  delete[] newTopic;

  // Read the message type
  memcpy(&this->type, _buffer, sizeof(this->type));
  _buffer += sizeof(this->type);

  // Read the flags
  memcpy(&this->flags, _buffer, sizeof(this->flags));
  _buffer += sizeof(this->flags);

  this->UpdateHeaderLength();
  return this->GetHeaderLength();
}

//////////////////////////////////////////////////
void transport::Header::UpdateHeaderLength()
{
  this->headerLength = sizeof(this->version) + sizeof(this->guid) +
                       sizeof(this->topicLength) + this->topic.size() +
                       sizeof(this->type) + sizeof(this->flags);
}

//////////////////////////////////////////////////
transport::AdvMsg::AdvMsg()
  :  msgLength(0)
{
}

//////////////////////////////////////////////////
transport::AdvMsg::AdvMsg(const Header &_header,
                          const std::string &_address)
{
  this->SetHeader(_header);
  this->SetAddress(_address);
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
transport::Header& transport::AdvMsg::GetHeader()
{
  return this->header;
}

//////////////////////////////////////////////////
uint16_t transport::AdvMsg::GetAddressLength() const
{
  return this->addressLength;
}

//////////////////////////////////////////////////
std::string transport::AdvMsg::GetAddress() const
{
  return this->address;
}

//////////////////////////////////////////////////
void transport::AdvMsg::SetHeader(const Header &_header)
{
  this->header = _header;
  if (_header.GetType() != ADV && _header.GetType() != ADV_SVC)
    std::cerr << "You're trying to use a "
              << msgTypesStr[_header.GetType()] << " header inside an ADV"
              << " or ADV_SVC. Are you sure you want to do this?\n";
}

//////////////////////////////////////////////////
void transport::AdvMsg::SetAddress(const std::string &_address)
{
  this->address = _address;
  this->addressLength = this->address.size();
  this->UpdateMsgLength();
}

//////////////////////////////////////////////////
size_t transport::AdvMsg::GetMsgLength()
{
  return this->header.GetHeaderLength() + sizeof(this->addressLength) +
         this->address.size();
}

//////////////////////////////////////////////////
void transport::AdvMsg::PrintBody()
{
  std::cout << "\tBody:" << std::endl;
  std::cout << "\t\tAddr size: " << this->GetAddressLength() << std::endl;
  std::cout << "\t\tAddress: " << this->GetAddress() << std::endl;
}

//////////////////////////////////////////////////
size_t transport::AdvMsg::Pack(char *_buffer)
{
  if (this->msgLength == 0)
  return 0;

  this->GetHeader().Pack(_buffer);
  _buffer += this->GetHeader().GetHeaderLength();

  memcpy(_buffer, &this->addressLength, sizeof(this->addressLength));
  _buffer += sizeof(this->addressLength);
  memcpy(_buffer, this->address.data(), this->address.size());

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t transport::AdvMsg::UnpackBody(char *_buffer)
{
  // Read the address length
  memcpy(&this->addressLength, _buffer, sizeof(this->addressLength));
  _buffer += sizeof(this->addressLength);

  // Read the address
  char *newAddress = new char[this->addressLength + 1];
  memcpy(newAddress, _buffer, this->addressLength);
  newAddress[this->addressLength] = '\0';
  this->address = newAddress;

  this->UpdateMsgLength();

  return sizeof(this->addressLength) + this->address.size();
}

//////////////////////////////////////////////////
void transport::AdvMsg::UpdateMsgLength()
{
  this->msgLength = this->GetHeader().GetHeaderLength() +
    sizeof(this->addressLength) + this->address.size();
}
