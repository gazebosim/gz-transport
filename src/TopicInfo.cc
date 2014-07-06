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

#include <string>
#include "ignition/transport/TopicInfo.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
TopicInfo::TopicInfo(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const Scope &_scope)
  : topic(_topic),
    addr(_addr),
    ctrl(_ctrl),
    pUuid(_pUuid),
    nUuid(_nUuid),
    scope(_scope)
{
}

//////////////////////////////////////////////////
std::string TopicInfo::GetTopic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
std::string TopicInfo::GetAddr() const
{
  return this->addr;
}
//////////////////////////////////////////////////
std::string TopicInfo::GetCtrl() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
std::string TopicInfo::GetPUuid() const
{
  return this->pUuid;
}

//////////////////////////////////////////////////
std::string TopicInfo::GetNUuid() const
{
  return this->nUuid;
}

//////////////////////////////////////////////////
Scope TopicInfo::GetScope() const
{
  return this->scope;
}

//////////////////////////////////////////////////
void TopicInfo::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
void TopicInfo::SetAddr(const std::string &_addr)
{
  this->addr = _addr;
}

//////////////////////////////////////////////////
void TopicInfo::SetCtrl(const std::string &_ctrl)
{
  this->ctrl = _ctrl;
}

//////////////////////////////////////////////////
void TopicInfo::SetPUuid(const std::string &_pUuid)
{
  this->pUuid = _pUuid;
}

//////////////////////////////////////////////////
void TopicInfo::SetNUuid(const std::string &_nUuid)
{
  this->nUuid = _nUuid;
}

//////////////////////////////////////////////////
void TopicInfo::SetScope(const Scope _scope)
{
  this->scope = _scope;
}

//////////////////////////////////////////////////
MsgTopicInfo::MsgTopicInfo(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const std::string &_msgType,
  const size_t _msgHash, const Scope &_scope)
  : TopicInfo(_topic, _addr, _ctrl, _pUuid, _nUuid, _scope),
    msgType(_msgType),
    msgHash(_msgHash)
{
}

//////////////////////////////////////////////////
std::string MsgTopicInfo::GetMsgType() const
{
  return this->msgType;
}

//////////////////////////////////////////////////
size_t MsgTopicInfo::GetMsgHash() const
{
  return this->msgHash;
}
//////////////////////////////////////////////////
void MsgTopicInfo::SetMsgType(const std::string &_msgType)
{
  this->msgType = _msgType;
}

//////////////////////////////////////////////////
void MsgTopicInfo::SetMsgHash(const size_t _msgHash)
{
  this->msgHash = _msgHash;
}

//////////////////////////////////////////////////
SrvTopicInfo::SrvTopicInfo(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const std::string &_reqType, const size_t _reqHash,
  const std::string &_repType, const size_t _repHash, const Scope &_scope)
  : TopicInfo(_topic, _addr, _ctrl, _pUuid, _nUuid, _scope),
    reqType(_reqType),
    reqHash(_reqHash),
    repType(_repType),
    repHash(_repHash)
{
}

//////////////////////////////////////////////////
std::string SrvTopicInfo::GetReqType() const
{
  return this->reqType;
}

//////////////////////////////////////////////////
size_t SrvTopicInfo::GetReqHash() const
{
  return this->reqHash;
}

//////////////////////////////////////////////////
std::string SrvTopicInfo::GetRepType() const
{
  return this->repType;
}

//////////////////////////////////////////////////
size_t SrvTopicInfo::GetRepHash() const
{
  return this->repHash;
}

//////////////////////////////////////////////////
void SrvTopicInfo::SetReqType(const std::string &_reqType)
{
  this->reqType = _reqType;
}

//////////////////////////////////////////////////
void SrvTopicInfo::SetReqHash(const size_t _reqHash)
{
  this->reqHash = _reqHash;
}

//////////////////////////////////////////////////
void SrvTopicInfo::SetRepType(const std::string &_repType)
{
  this->repType = _repType;
}

//////////////////////////////////////////////////
void SrvTopicInfo::SetRepHash(const size_t _repHash)
{
  this->repHash = _repHash;
}
