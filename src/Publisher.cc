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

#include "gz/transport/AdvertiseOptions.hh"
#include "gz/transport/Helpers.hh"
#include "gz/transport/Publisher.hh"
#include "gz/transport/NodeShared.hh"
#include "gz/transport/SubscriptionHandler.hh"

using namespace gz;
using namespace transport;

//////////////////////////////////////////////////
Publisher::Publisher(const std::string &_topic, const std::string &_addr,
  const std::string &_pUuid, const std::string &_nUuid,
  const AdvertiseOptions &_opts)
  : topic(_topic),
    addr(_addr),
    pUuid(_pUuid),
    nUuid(_nUuid),
    opts(_opts)
{
}

//////////////////////////////////////////////////
Publisher::Publisher(const Publisher &_other)
  : Publisher()
{
  (*this) = _other;
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
const AdvertiseOptions& Publisher::Options() const
{
  return this->opts;
}

//////////////////////////////////////////////////
void Publisher::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
void Publisher::SetAddr(const std::string &_addr)
{
  this->addr = _addr;
}

//////////////////////////////////////////////////
void Publisher::SetPUuid(const std::string &_pUuid)
{
  this->pUuid = _pUuid;
}

//////////////////////////////////////////////////
void Publisher::SetNUuid(const std::string &_nUuid)
{
  this->nUuid = _nUuid;
}

//////////////////////////////////////////////////
void Publisher::SetOptions(const AdvertiseOptions &_opts)
{
  this->opts = _opts;
}
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

//////////////////////////////////////////////////
size_t Publisher::Pack(char *_buffer) const
{
  size_t len = this->PackInternal(_buffer);
  if (len == 0)
    return 0;

  _buffer += len;

  // Pack the options.
  size_t optsLen = this->opts.Pack(_buffer);
  if (optsLen == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t Publisher::PackInternal(char *_buffer) const
{
  if (this->topic.empty() || this->addr.empty() ||
      this->pUuid.empty() || this->nUuid.empty())
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

  // Pack the topic length.
  uint16_t topicLength = static_cast<uint16_t>(this->topic.size());
  memcpy(_buffer, &topicLength, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Pack the topic.
  memcpy(_buffer, this->topic.data(), static_cast<size_t>(topicLength));
  _buffer += topicLength;

  // Pack the zeromq address length.
  uint16_t addrLength = static_cast<uint16_t>(this->addr.size());
  memcpy(_buffer, &addrLength, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Pack the zeromq address.
  memcpy(_buffer, this->addr.data(), static_cast<size_t>(addrLength));
  _buffer += addrLength;

  // Pack the process UUID length.
  uint16_t pUuidLength = static_cast<uint16_t>(this->pUuid.size());
  memcpy(_buffer, &pUuidLength, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Pack the process UUID.
  memcpy(_buffer, this->pUuid.data(), static_cast<size_t>(pUuidLength));
  _buffer += pUuidLength;

  // Pack the node UUID length.
  uint16_t nUuidLength = static_cast<uint16_t>(this->nUuid.size());
  memcpy(_buffer, &nUuidLength, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Pack the node UUID.
  memcpy(_buffer, this->nUuid.data(), static_cast<size_t>(nUuidLength));

  return this->MsgLengthInternal();
}

//////////////////////////////////////////////////
size_t Publisher::Unpack(const char *_buffer)
{
  size_t len = this->UnpackInternal(_buffer);
  if (len == 0)
    return 0;

  _buffer += len;

  // Unpack the options.
  size_t optsLen = this->opts.Unpack(_buffer);
  if (optsLen == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t Publisher::UnpackInternal(const char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "Publisher::Unpack() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  // Unpack the topic length.
  uint16_t topicLength;
  memcpy(&topicLength, _buffer, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Unpack the topic.
  this->topic = std::string(_buffer, _buffer + topicLength);
  _buffer += topicLength;

  // Unpack the zeromq address length.
  uint16_t addrLength;
  memcpy(&addrLength, _buffer, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Unpack the zeromq address.
  this->addr = std::string(_buffer, _buffer + addrLength);
  _buffer += addrLength;

  // Unpack the process UUID length.
  uint16_t pUuidLength;
  memcpy(&pUuidLength, _buffer, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Unpack the process UUID.
  this->pUuid = std::string(_buffer, _buffer + pUuidLength);
  _buffer += pUuidLength;

  // Unpack the node UUID length.
  uint16_t nUuidLength;
  memcpy(&nUuidLength, _buffer, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Unpack the node UUID.
  this->nUuid = std::string(_buffer, _buffer + nUuidLength);

  return this->MsgLengthInternal();
}

//////////////////////////////////////////////////
size_t Publisher::MsgLength() const
{
  return this->MsgLengthInternal() +
         this->opts.MsgLength();
}
#ifndef _WIN32
  #pragma GCC diagnostic pop
#endif

//////////////////////////////////////////////////
void Publisher::FillDiscovery(msgs::Discovery &_msg) const
{
  msgs::Discovery::Publisher *pub = _msg.mutable_pub();
  pub->set_topic(this->Topic());
  pub->set_address(this->Addr());
  pub->set_process_uuid(this->PUuid());
  pub->set_node_uuid(this->NUuid());

  switch (this->opts.Scope())
  {
    case Scope_t::PROCESS:
      pub->set_scope(msgs::Discovery::Publisher::PROCESS);
      break;
    case Scope_t::HOST:
      pub->set_scope(msgs::Discovery::Publisher::HOST);
      break;
    default:
    case Scope_t::ALL:
      pub->set_scope(msgs::Discovery::Publisher::ALL);
      break;
  }
}

//////////////////////////////////////////////////
void Publisher::SetFromDiscovery(const msgs::Discovery &_msg)
{
  this->topic = _msg.pub().topic();
  this->addr = _msg.pub().address();
  this->pUuid = _msg.pub().process_uuid();
  this->nUuid = _msg.pub().node_uuid();

  switch (_msg.pub().scope())
  {
    case msgs::Discovery::Publisher::PROCESS:
      this->opts.SetScope(Scope_t::PROCESS);
      break;
    case msgs::Discovery::Publisher::HOST:
      this->opts.SetScope(Scope_t::HOST);
      break;
    default:
    case msgs::Discovery::Publisher::ALL:
      this->opts.SetScope(Scope_t::ALL);
      break;
  }
}

//////////////////////////////////////////////////
size_t Publisher::MsgLengthInternal() const
{
  return sizeof(uint16_t) + this->topic.size() +
         sizeof(uint16_t) + this->addr.size()  +
         sizeof(uint16_t) + this->pUuid.size() +
         sizeof(uint16_t) + this->nUuid.size();
}

//////////////////////////////////////////////////
bool Publisher::operator==(const Publisher &_pub) const
{
  return this->topic == _pub.topic && this->addr == _pub.addr &&
    this->pUuid == _pub.pUuid && this->nUuid == _pub.nUuid &&
    this->Options() == _pub.Options();
}

//////////////////////////////////////////////////
bool Publisher::operator!=(const Publisher &_pub) const
{
  return !(*this == _pub);
}

//////////////////////////////////////////////////
Publisher &Publisher::operator=(const Publisher &_other)
{
  this->SetTopic(_other.Topic());
  this->SetAddr(_other.Addr());
  this->SetPUuid(_other.PUuid());
  this->SetNUuid(_other.NUuid());
  this->SetOptions(_other.Options());
  return *this;
}

//////////////////////////////////////////////////
MessagePublisher::MessagePublisher(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const std::string &_msgTypeName,
  const AdvertiseMessageOptions &_opts)
  : Publisher(_topic, _addr, _pUuid, _nUuid, _opts),
    ctrl(_ctrl),
    msgTypeName(_msgTypeName),
    msgOpts(_opts)
{
}

//////////////////////////////////////////////////
MessagePublisher::MessagePublisher(const MessagePublisher &_other)
  : MessagePublisher()
{
  (*this) = _other;
}

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
//////////////////////////////////////////////////
size_t MessagePublisher::Pack(char *_buffer) const
{
  if (this->ctrl.empty() || this->msgTypeName.empty())
  {
    std::cerr << "MessagePublisher::Pack() error: You're trying to pack an "
              << "incomplete MessagePublisher:" << std::endl << *this;
    return 0;
  }

  // Pack the common part of any Publisher message except the options.
  size_t len = Publisher::PackInternal(_buffer);
  if (len == 0)
    return 0;

  _buffer += len;

  // Pack the zeromq control address length.
  uint16_t ctrlLength = static_cast<uint16_t>(this->ctrl.size());
  memcpy(_buffer, &ctrlLength, sizeof(ctrlLength));
  _buffer += sizeof(ctrlLength);

  // Pack the zeromq control address.
  memcpy(_buffer, this->ctrl.data(), static_cast<size_t>(ctrlLength));
  _buffer += ctrlLength;

  // Pack the type name length.
  uint16_t typeNameLength = static_cast<uint16_t>(this->msgTypeName.size());
  memcpy(_buffer, &typeNameLength, sizeof(typeNameLength));
  _buffer += sizeof(typeNameLength);

  // Pack the type name.
  memcpy(_buffer, this->msgTypeName.data(),
    static_cast<size_t>(typeNameLength));
  _buffer += typeNameLength;

  // Pack the options.
  size_t optsLen = this->msgOpts.Pack(_buffer);
  if (optsLen == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t MessagePublisher::Unpack(const char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "MessagePublisher::UnpackBody() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  // Unpack the common part of any Publisher message except the options.
  size_t len = Publisher::UnpackInternal(_buffer);
  if (len == 0)
    return 0;

  _buffer += len;

  // Unpack the zeromq control address length.
  uint16_t ctrlLength;
  memcpy(&ctrlLength, _buffer, sizeof(ctrlLength));
  _buffer += sizeof(ctrlLength);

  // Unpack the zeromq control address.
  this->ctrl = std::string(_buffer, _buffer + ctrlLength);
  _buffer += ctrlLength;

  // Unpack the type name length.
  uint16_t typeNameLength;
  memcpy(&typeNameLength, _buffer, sizeof(typeNameLength));
  _buffer += sizeof(typeNameLength);

  // Unpack the type name.
  this->msgTypeName = std::string(_buffer, _buffer + typeNameLength);
  _buffer += typeNameLength;

  // Unpack the options.
  size_t optsLen = this->msgOpts.Unpack(_buffer);
  if (optsLen == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t MessagePublisher::MsgLength() const
{
  return Publisher::MsgLengthInternal()              +
         sizeof(uint16_t) + this->ctrl.size()        +
         sizeof(uint16_t) + this->msgTypeName.size() +
         this->msgOpts.MsgLength();
}
#ifndef _WIN32
  #pragma GCC diagnostic pop
#endif


//////////////////////////////////////////////////
std::string MessagePublisher::Ctrl() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
void MessagePublisher::SetCtrl(const std::string &_ctrl)
{
  this->ctrl = _ctrl;
}

//////////////////////////////////////////////////
std::string MessagePublisher::MsgTypeName() const
{
  return this->msgTypeName;
}

//////////////////////////////////////////////////
void MessagePublisher::SetMsgTypeName(const std::string &_msgTypeName)
{
  this->msgTypeName = _msgTypeName;
}

//////////////////////////////////////////////////
const AdvertiseMessageOptions& MessagePublisher::Options() const
{
  return this->msgOpts;
}

//////////////////////////////////////////////////
void MessagePublisher::SetOptions(const AdvertiseMessageOptions &_opts)
{
  this->msgOpts = _opts;
}

//////////////////////////////////////////////////
void MessagePublisher::FillDiscovery(msgs::Discovery &_msg) const
{
  Publisher::FillDiscovery(_msg);
  msgs::Discovery::Publisher *pub = _msg.mutable_pub();

  // Message options
  pub->mutable_msg_pub()->set_ctrl(this->Ctrl());
  pub->mutable_msg_pub()->set_msg_type(this->MsgTypeName());
  pub->mutable_msg_pub()->set_throttled(this->msgOpts.Throttled());
  pub->mutable_msg_pub()->set_msgs_per_sec(this->msgOpts.MsgsPerSec());
}

//////////////////////////////////////////////////
void MessagePublisher::SetFromDiscovery(const msgs::Discovery &_msg)
{
  Publisher::SetFromDiscovery(_msg);
  this->ctrl = _msg.pub().msg_pub().ctrl();
  this->msgTypeName = _msg.pub().msg_pub().msg_type();
  this->msgOpts.SetScope(Publisher::Options().Scope());
  if (!_msg.pub().msg_pub().throttled())
    this->msgOpts.SetMsgsPerSec(kUnthrottled);
  else
    this->msgOpts.SetMsgsPerSec(_msg.pub().msg_pub().msgs_per_sec());
}

//////////////////////////////////////////////////
bool MessagePublisher::operator==(const MessagePublisher &_pub) const
{
  return Publisher::operator==(_pub)      &&
    this->ctrl == _pub.ctrl               &&
    this->msgTypeName == _pub.msgTypeName;
}

//////////////////////////////////////////////////
bool MessagePublisher::operator!=(const MessagePublisher &_pub) const
{
  return !(*this == _pub);
}

//////////////////////////////////////////////////
MessagePublisher &MessagePublisher::operator=(const MessagePublisher &_other)
{
  Publisher::operator=(_other);
  this->SetCtrl(_other.Ctrl());
  this->SetMsgTypeName(_other.MsgTypeName());
  this->SetOptions(_other.Options());
  return *this;
}

//////////////////////////////////////////////////
ServicePublisher::ServicePublisher(const std::string &_topic,
  const std::string &_addr, const std::string &_socketId,
  const std::string &_pUuid, const std::string &_nUuid,
  const std::string &_reqType, const std::string &_repType,
  const AdvertiseServiceOptions &_opts)
  : Publisher(_topic, _addr, _pUuid, _nUuid, _opts),
    socketId(_socketId),
    reqTypeName(_reqType),
    repTypeName(_repType),
    srvOpts(_opts)
{
}

//////////////////////////////////////////////////
ServicePublisher::ServicePublisher(const ServicePublisher &_other)
  : ServicePublisher()
{
  (*this) = _other;
}

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
//////////////////////////////////////////////////
size_t ServicePublisher::Pack(char *_buffer) const
{
  if (this->socketId.empty() || this->reqTypeName.empty() ||
      this->repTypeName.empty())
  {
    std::cerr << "ServicePublisher::Pack() error: You're trying to pack an "
              << "incomplete ServicePublisher:" << std::endl << *this;
    return 0;
  }

  // Pack the common part of any Publisher message.
  size_t len = Publisher::PackInternal(_buffer);
  if (len == 0)
    return 0;

  _buffer += len;

  // Pack the socket ID length.
  uint16_t socketIdLength = static_cast<uint16_t>(this->socketId.size());
  memcpy(_buffer, &socketIdLength, sizeof(socketIdLength));
  _buffer += sizeof(socketIdLength);

  // Pack the socket ID.
  memcpy(_buffer, this->socketId.data(), static_cast<size_t>(socketIdLength));
  _buffer += socketIdLength;

  // Pack the request type length.
  uint16_t reqTypeLength = static_cast<uint16_t>(this->reqTypeName.size());
  memcpy(_buffer, &reqTypeLength, sizeof(reqTypeLength));
  _buffer += sizeof(reqTypeLength);

  // Pack the request type.
  memcpy(_buffer, this->reqTypeName.data(), static_cast<size_t>(reqTypeLength));
  _buffer += reqTypeLength;

  // Pack the response type length.
  uint16_t repTypeLength = static_cast<uint16_t>(this->repTypeName.size());
  memcpy(_buffer, &repTypeLength, sizeof(repTypeLength));
  _buffer += sizeof(repTypeLength);

  // Pack the response.
  memcpy(_buffer, this->repTypeName.data(), static_cast<size_t>(repTypeLength));
  _buffer += repTypeLength;

  // Pack the options.
  size_t optsLen = this->srvOpts.Pack(_buffer);
  if (optsLen == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t ServicePublisher::Unpack(const char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "MessagePublisher::Unpack() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  // Unpack the common part of any Publisher message.
  size_t len = Publisher::UnpackInternal(_buffer);
  if (len == 0)
    return 0;

  _buffer += len;

  // Unpack the socket ID length.
  uint16_t socketIdLength;
  memcpy(&socketIdLength, _buffer, sizeof(socketIdLength));
  _buffer += sizeof(socketIdLength);

  // Unpack the socket ID.
  this->socketId = std::string(_buffer, _buffer + socketIdLength);
  _buffer += socketIdLength;

  // Unpack the request type length.
  uint16_t reqTypeLength;
  memcpy(&reqTypeLength, _buffer, sizeof(reqTypeLength));
  _buffer += sizeof(reqTypeLength);

  // Unpack the request type.
  this->reqTypeName = std::string(_buffer, _buffer + reqTypeLength);
  _buffer += reqTypeLength;

  // Unpack the response type length.
  uint16_t repTypeLength;
  memcpy(&repTypeLength, _buffer, sizeof(repTypeLength));
  _buffer += sizeof(repTypeLength);

  // Unpack the response type.
  this->repTypeName = std::string(_buffer, _buffer + repTypeLength);
  _buffer += repTypeLength;

  // Unpack the options.
  size_t optsLen = this->srvOpts.Unpack(_buffer);
  if (optsLen == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t ServicePublisher::MsgLength() const
{
  return Publisher::MsgLengthInternal() +
         sizeof(uint16_t) + this->socketId.size()    +
         sizeof(uint16_t) + this->reqTypeName.size() +
         sizeof(uint16_t) + this->repTypeName.size() +
         this->srvOpts.MsgLength();
}
#ifndef _WIN32
  #pragma GCC diagnostic pop
#endif

//////////////////////////////////////////////////
std::string ServicePublisher::SocketId() const
{
  return this->socketId;
}

//////////////////////////////////////////////////
void ServicePublisher::SetSocketId(const std::string &_socketId)
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
void ServicePublisher::SetReqTypeName(const std::string &_reqTypeName)
{
  this->reqTypeName = _reqTypeName;
}

//////////////////////////////////////////////////
void ServicePublisher::SetRepTypeName(const std::string &_repTypeName)
{
  this->repTypeName = _repTypeName;
}

//////////////////////////////////////////////////
const AdvertiseServiceOptions& ServicePublisher::Options() const
{
  return this->srvOpts;
}

//////////////////////////////////////////////////
void ServicePublisher::SetOptions(const AdvertiseServiceOptions &_opts)
{
  this->srvOpts = _opts;
}

//////////////////////////////////////////////////
void ServicePublisher::FillDiscovery(msgs::Discovery &_msg) const
{
  Publisher::FillDiscovery(_msg);
  msgs::Discovery::Publisher *pub = _msg.mutable_pub();

  // Service publisher info
  pub->mutable_srv_pub()->set_socket_id(this->SocketId());
  pub->mutable_srv_pub()->set_request_type(this->ReqTypeName());
  pub->mutable_srv_pub()->set_response_type(this->RepTypeName());
}

//////////////////////////////////////////////////
void ServicePublisher::SetFromDiscovery(const msgs::Discovery &_msg)
{
  Publisher::SetFromDiscovery(_msg);
  this->srvOpts.SetScope(Publisher::Options().Scope());
  this->socketId = _msg.pub().srv_pub().socket_id();
  this->reqTypeName = _msg.pub().srv_pub().request_type();
  this->repTypeName = _msg.pub().srv_pub().response_type();
}

//////////////////////////////////////////////////
bool ServicePublisher::operator==(const ServicePublisher &_srv) const
{
  return Publisher::operator==(_srv)      &&
    this->socketId == _srv.socketId       &&
    this->reqTypeName == _srv.reqTypeName &&
    this->repTypeName == _srv.repTypeName;
}

//////////////////////////////////////////////////
bool ServicePublisher::operator!=(const ServicePublisher &_srv) const
{
  return !(*this == _srv);
}
