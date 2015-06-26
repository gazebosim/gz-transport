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
#include <cstdint>
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
  this->Version(_version);
  this->PUuid(_pUuid);
  this->Type(_type);
  this->Flags(_flags);
}

//////////////////////////////////////////////////
uint16_t Header::Version() const
{
  return this->version;
}

//////////////////////////////////////////////////
std::string Header::PUuid() const
{
  return this->pUuid;
}

//////////////////////////////////////////////////
uint8_t Header::Type() const
{
  return this->type;
}

//////////////////////////////////////////////////
uint16_t Header::Flags() const
{
  return this->flags;
}

//////////////////////////////////////////////////
void Header::Version(const uint16_t _version)
{
  this->version = _version;
}

//////////////////////////////////////////////////
void Header::PUuid(const std::string &_pUuid)
{
  this->pUuid = _pUuid;
}

//////////////////////////////////////////////////
void Header::Type(const uint8_t _type)
{
  this->type = _type;
}

//////////////////////////////////////////////////
void Header::Flags(const uint16_t _flags)
{
  this->flags = _flags;
}

//////////////////////////////////////////////////
bool Header::Pack(std::vector<char> &_buffer) const
{
  if ((this->version == 0) || (this->pUuid == "") ||
      (this->type == Uninitialized))
  {
    std::cerr << "Header::Pack() error: You're trying to pack an "
              << "incomplete header" << std::endl;
    return false;
  }

  msgs::HeaderData message;
  message.set_version(this->version);
  message.set_puuid(this->pUuid);
  message.set_type(static_cast<int>(this->type));
  message.set_flags(this->flags);

  // Pack Header data.
  return serialize(message, _buffer);
}

//////////////////////////////////////////////////
bool Header::Unpack(const std::vector<char> &_buffer)
{
  // Empty buffer.
  if (_buffer.empty())
  {
    std::cerr << "Header::Unpack() error: Empty input buffer"
              << std::endl;
    return false;
  }

  msgs::HeaderData message;

  // Unpack Header data
  if (!unserialize(_buffer, message))
    return false;

  this->version = message.version();
  this->pUuid = message.puuid();
  this->type = message.type();
  this->flags = message.flags();

  return true;
}

//////////////////////////////////////////////////
Message::Message(const Header &_header)
{
  this->SetHeader(_header);
}

//////////////////////////////////////////////////
Header Message::GetHeader() const
{
  return this->header;
}

//////////////////////////////////////////////////
void Message::SetHeader(const Header &_header)
{
  this->header = _header;
}

//////////////////////////////////////////////////
bool Message::Pack(std::vector<char> &_buffer) const
{
  std::vector<char> v;
  if (!this->header.Pack(v))
  {
    std::cerr << "Message::Pack() error packing header" << std::endl;
    return false;
  }

  msgs::HeaderData head;
  if (!unserialize(v, head))
    return false;

  msgs::MessageData message;
  message.mutable_header()->CopyFrom(head);

  // Pack Message data
  return serialize(message, _buffer);
}

//////////////////////////////////////////////////
bool Message::Unpack(const std::vector<char> &_buffer)
{
  // Empty buffer.
  if (_buffer.empty())
  {
    std::cerr << "Message::Unpack() error: Empty input buffer"
              << std::endl;
    return false;
  }

  msgs::MessageData message;

  // Unpack Message data.
  if (!unserialize(_buffer, message))
    return false;

  // Unpack the header.
  std::vector<char> headerBuffer;
  if (!serialize(message.header(), headerBuffer))
    return false;

  return this->header.Unpack(headerBuffer);
}

//////////////////////////////////////////////////
SubscriptionMsg::SubscriptionMsg(const Header &_header,
                                 const std::string &_topic)
  : Message(_header)
{
  this->Topic(_topic);
}

//////////////////////////////////////////////////
std::string SubscriptionMsg::Topic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
void SubscriptionMsg::Topic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
bool SubscriptionMsg::Pack(std::vector<char> &_buffer) const
{
  if (this->topic == "")
  {
    std::cerr << "SubscriptionMsg::Pack() error: You're trying to pack an "
              << "incomplete message" << std::endl;
    return false;
  }

  if (!Message::Pack(_buffer))
    return false;

  msgs::MessageData message;
  if (!unserialize(_buffer, message))
    return false;

  // Add the topic.
  message.set_topic(this->topic);

  return serialize(message, _buffer);
}

//////////////////////////////////////////////////
bool SubscriptionMsg::Unpack(const std::vector<char> &_buffer)
{
  // Empty buffer.
  if (_buffer.empty())
  {
    std::cerr << "SubscriptionMsg::Unpack() error: Empty input buffer"
              << std::endl;
    return false;
  }

  if (!Message::Unpack(_buffer))
    return false;

  msgs::MessageData message;
  if (!unserialize(_buffer, message))
    return false;

  this->topic = message.topic();

  return true;
}
