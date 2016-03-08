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

#include <limits.h>
#include <iostream>
#include <string>
#include <vector>

#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
/// \brief Check the getters and setters.
TEST(PacketTest, BasicHeaderAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;
  Header header(version, pUuid, AdvType);

  // Check Header getters.
  EXPECT_EQ(version, header.Version());
  EXPECT_EQ(pUuid, header.PUuid());
  EXPECT_EQ(header.Type(), AdvType);
  EXPECT_EQ(header.Flags(), 0);
  int headerLength = static_cast<int>(sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags()));
  EXPECT_EQ(header.HeaderLength(), headerLength);

  // Check Header setters.
  pUuid = "Different-process-UUID-1";
  header.SetPUuid(pUuid);
  EXPECT_EQ(header.PUuid(), pUuid);
  header.SetType(SubType);
  EXPECT_EQ(header.Type(), SubType);
  header.SetFlags(1);
  EXPECT_EQ(header.Flags(), 1);
  headerLength = static_cast<int>(sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags()));
  EXPECT_EQ(header.HeaderLength(), headerLength);

  // Check << operator
  std::ostringstream output;
  output << header;
  std::string expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 1\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: SUBSCRIBE\n"
    "\tFlags: 1\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of a header.
TEST(PacketTest, HeaderIO)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  // Try to pack an empty header.
  Header emptyHeader;
  std::vector<char> buffer(emptyHeader.HeaderLength());
  EXPECT_EQ(emptyHeader.Pack(&buffer[0]), 0u);

  // Pack a Header.
  Header header(version, pUuid, AdvSrvType, 2);

  buffer.resize(header.HeaderLength());
  int bytes = static_cast<int>(header.Pack(&buffer[0]));
  EXPECT_EQ(bytes, header.HeaderLength());

  // Unpack the Header.
  Header otherHeader;
  otherHeader.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Header remains the same.
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());
  EXPECT_EQ(header.HeaderLength(), otherHeader.HeaderLength());

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherHeader.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherHeader.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV message.
TEST(PacketTest, BasicSubscriptionAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  Header otherHeader(version, pUuid, SubType, 3);

  std::string topic = "topic_test";
  SubscriptionMsg subMsg(otherHeader, topic);

  // Check Sub getters.
  EXPECT_EQ(subMsg.Topic(), topic);

  size_t msgLength = subMsg.Header().HeaderLength() +
    sizeof(uint64_t) + topic.size();
  EXPECT_EQ(subMsg.MsgLength(), msgLength);

  // Check Sub setters.
  topic = "a_new_topic_test";
  subMsg.SetTopic(topic);
  EXPECT_EQ(subMsg.Topic(), topic);

  // Check << operator
  std::ostringstream output;
  output << subMsg;
  std::string expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 1\n"
    "\tProcess UUID: Process-UUID-1\n"
    "\tType: SUBSCRIBE\n"
    "\tFlags: 3\n"
    "Body:\n"
    "\tTopic: [a_new_topic_test]\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of a SUB message.
TEST(PacketTest, SubscriptionIO)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  // Try to pack an empty SubscriptionMsg.
  SubscriptionMsg emptyMsg;
  std::vector<char> buffer(emptyMsg.MsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Pack a SubscriptionMsg with an empty topic.
  Header otherHeader(version, pUuid, SubType, 3);
  SubscriptionMsg incompleteMsg(otherHeader, "");
  buffer.resize(incompleteMsg.MsgLength());
  EXPECT_EQ(0u, incompleteMsg.Pack(&buffer[0]));

  // Pack a SubscriptionMsg.
  std::string topic = "topic_test";
  SubscriptionMsg subMsg(otherHeader, topic);
  buffer.resize(subMsg.MsgLength());
  size_t bytes = subMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, subMsg.MsgLength());

  // Unpack a SubscriptionMsg.
  Header header;
  SubscriptionMsg otherSubMsg;
  size_t headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, static_cast<size_t>(header.HeaderLength()));
  otherSubMsg.SetHeader(header);
  char *pBody = &buffer[0] + header.HeaderLength();
  size_t bodyBytes = otherSubMsg.Unpack(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherSubMsg.Topic(), subMsg.Topic());
  EXPECT_EQ(otherSubMsg.MsgLength() -
            otherSubMsg.Header().HeaderLength(), subMsg.MsgLength() -
            subMsg.Header().HeaderLength());
  EXPECT_EQ(bodyBytes, otherSubMsg.MsgLength() -
            otherSubMsg.Header().HeaderLength());

  // Try to pack a SubscriptionMsg passing a NULL buffer.
  EXPECT_EQ(otherSubMsg.Pack(nullptr), 0u);

  // Try to unpack a SubscriptionMsg passing a NULL buffer.
  EXPECT_EQ(otherSubMsg.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV message.
TEST(PacketTest, BasicAdvertiseMsgAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  Header otherHeader(version, pUuid, AdvType, 3);

  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string procUuid = "procUUID";
  std::string nodeUuid = "nodeUUID";
  Scope_t scope = Scope_t::ALL;
  std::string typeName = "StringMsg";
  MessagePublisher pub(topic, addr, ctrl, procUuid, nodeUuid, scope, typeName);
  AdvertiseMessage<MessagePublisher> advMsg(otherHeader, pub);

  // Check AdvertiseMsg getters.
  Header header = advMsg.Header();
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());
  EXPECT_EQ(header.HeaderLength(), otherHeader.HeaderLength());

  EXPECT_EQ(advMsg.Publisher().Topic(), topic);
  EXPECT_EQ(advMsg.Publisher().Addr(), addr);
  EXPECT_EQ(advMsg.Publisher().Ctrl(), ctrl);
  EXPECT_EQ(advMsg.Publisher().NUuid(), nodeUuid);
  EXPECT_EQ(advMsg.Publisher().Scope(), scope);
  EXPECT_EQ(advMsg.Publisher().MsgTypeName(), typeName);

  size_t msgLength = advMsg.Header().HeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + ctrl.size() +
    sizeof(uint64_t) + procUuid.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t)  +
    sizeof(uint64_t) + typeName.size();
  EXPECT_EQ(advMsg.MsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseMsg setters.
  Header anotherHeader(version + 1, pUuid, AdvSrvType, 3);
  EXPECT_EQ(anotherHeader.Version(), version + 1);
  advMsg.SetHeader(anotherHeader);
  EXPECT_EQ(advMsg.Header().Version(), version + 1);
  header = advMsg.Header();
  EXPECT_EQ(header.Version(), version + 1);
  EXPECT_EQ(header.PUuid(), anotherHeader.PUuid());
  EXPECT_EQ(header.Type(), AdvSrvType);
  EXPECT_EQ(header.Flags(), 3);
  size_t headerLength = sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags());
  EXPECT_EQ(static_cast<size_t>(header.HeaderLength()), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  procUuid = "procUUID";
  nodeUuid = "nodeUUID2";
  scope = Scope_t::HOST;
  typeName = "Int";
  advMsg.Publisher().SetTopic(topic);
  EXPECT_EQ(advMsg.Publisher().Topic(), topic);
  advMsg.Publisher().SetAddr(addr);
  EXPECT_EQ(advMsg.Publisher().Addr(), addr);
  advMsg.Publisher().SetPUuid(procUuid);
  EXPECT_EQ(advMsg.Publisher().PUuid(), procUuid);
  advMsg.Publisher().SetCtrl(ctrl);
  EXPECT_EQ(advMsg.Publisher().Ctrl(), ctrl);
  advMsg.Publisher().SetNUuid(nodeUuid);
  EXPECT_EQ(advMsg.Publisher().NUuid(), nodeUuid);
  advMsg.Publisher().SetScope(scope);
  EXPECT_EQ(advMsg.Publisher().Scope(), scope);
  advMsg.Publisher().SetMsgTypeName(typeName);
  EXPECT_EQ(advMsg.Publisher().MsgTypeName(), typeName);

  // Check << operator
  std::ostringstream output;
  output << advMsg;
  std::string expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: ADV_SRV\n"
    "\tFlags: 3\n"
    "Publisher:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tProcess UUID: procUUID\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Host\n"
    "\tControl address: inproc://control\n"
    "\tMessage type: Int\n";

  EXPECT_EQ(output.str(), expectedOutput);

  advMsg.Publisher().SetScope(Scope_t::PROCESS);
  output.str("");
  output << advMsg;
  expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: ADV_SRV\n"
    "\tFlags: 3\n"
    "Publisher:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tProcess UUID: procUUID\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Process\n"
    "\tControl address: inproc://control\n"
    "\tMessage type: Int\n";

  EXPECT_EQ(output.str(), expectedOutput);

  // Check << operator
  advMsg.Publisher().SetScope(Scope_t::ALL);
  output.str("");
  output << advMsg;
  expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: ADV_SRV\n"
    "\tFlags: 3\n"
    "Publisher:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tProcess UUID: procUUID\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: All\n"
    "\tControl address: inproc://control\n"
    "\tMessage type: Int\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of an ADV message.
TEST(PacketTest, AdvertiseMsgIO)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;
  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string procUuid = "procUUID";
  std::string nodeUuid = "nodeUUID";
  Scope_t scope = Scope_t::HOST;
  std::string typeName = "StringMsg";

  // Try to pack an empty AdvMsg.
  AdvertiseMessage<MessagePublisher> emptyMsg;
  std::vector<char> buffer(emptyMsg.MsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvMsg (empty topic).
  Header otherHeader(version, pUuid, AdvType, 3);
  MessagePublisher publisherNoTopic("", addr, ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noTopicMsg(otherHeader, publisherNoTopic);
  buffer.resize(noTopicMsg.MsgLength());
  EXPECT_EQ(0u, noTopicMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty address).
  MessagePublisher publisherNoAddr(topic, "", ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noAddrMsg(otherHeader, publisherNoAddr);
  buffer.resize(noAddrMsg.MsgLength());
  EXPECT_EQ(0u, noAddrMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty node UUID).
  MessagePublisher publisherNoNUuid(topic, addr, ctrl, procUuid, "", scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noNodeUuidMsg(otherHeader,
    publisherNoNUuid);
  buffer.resize(noNodeUuidMsg.MsgLength());
  EXPECT_EQ(0u, noNodeUuidMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty message type name).
  MessagePublisher publisherNoMsgType(topic, addr, ctrl, procUuid, nodeUuid,
    scope, "");
  AdvertiseMessage<MessagePublisher> noTypeMsg(otherHeader, publisherNoMsgType);
  buffer.resize(noTypeMsg.MsgLength());
  EXPECT_EQ(0u, noTypeMsg.Pack(&buffer[0]));

  // Pack an AdvertiseMsg.
  MessagePublisher publisher(topic, addr, ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> advMsg(otherHeader, publisher);
  buffer.resize(advMsg.MsgLength());
  size_t bytes = advMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advMsg.MsgLength());

  // Unpack an AdvertiseMsg.
  Header header;
  AdvertiseMessage<MessagePublisher> otherAdvMsg;
  size_t headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, static_cast<size_t>(header.HeaderLength()));
  otherAdvMsg.SetHeader(header);
  char *pBody = &buffer[0] + header.HeaderLength();
  size_t bodyBytes = otherAdvMsg.Unpack(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvMsg.Publisher().Topic(), advMsg.Publisher().Topic());
  EXPECT_EQ(otherAdvMsg.Publisher().Addr(), advMsg.Publisher().Addr());
  EXPECT_EQ(otherAdvMsg.Publisher().Ctrl(), advMsg.Publisher().Ctrl());
  EXPECT_EQ(otherAdvMsg.Publisher().NUuid(), advMsg.Publisher().NUuid());
  EXPECT_EQ(otherAdvMsg.Publisher().Scope(), advMsg.Publisher().Scope());
  EXPECT_EQ(otherAdvMsg.Publisher().MsgTypeName(),
    advMsg.Publisher().MsgTypeName());
  EXPECT_EQ(otherAdvMsg.MsgLength(), advMsg.MsgLength());
  EXPECT_EQ(otherAdvMsg.MsgLength() -
            otherAdvMsg.Header().HeaderLength(), advMsg.MsgLength() -
            advMsg.Header().HeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvMsg.MsgLength() -
            otherAdvMsg.Header().HeaderLength());

  // Try to pack an AdvertiseMsg passing a NULL buffer.
  EXPECT_EQ(otherAdvMsg.Pack(nullptr), 0u);

  // Try to unpack an AdvertiseMsg passing a NULL buffer.
  EXPECT_EQ(otherAdvMsg.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV SRV message.
TEST(PacketTest, BasicAdvertiseSrvAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  Header otherHeader(version, pUuid, AdvType, 3);

  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string id = "socketID";
  std::string nodeUuid = "nodeUUID";
  Scope_t scope = Scope_t::ALL;
  std::string reqType = "StringMsg";
  std::string repType = "Int";

  ServicePublisher publisher(topic, addr, id, pUuid, nodeUuid, scope, reqType,
    repType);
  AdvertiseMessage<ServicePublisher> advSrv(otherHeader, publisher);

  // Check AdvertiseSrv getters.
  Header header = advSrv.Header();
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());
  EXPECT_EQ(header.HeaderLength(), otherHeader.HeaderLength());

  EXPECT_EQ(advSrv.Publisher().Topic(), topic);
  EXPECT_EQ(advSrv.Publisher().Addr(), addr);
  EXPECT_EQ(advSrv.Publisher().SocketId(), id);
  EXPECT_EQ(advSrv.Publisher().PUuid(), pUuid);
  EXPECT_EQ(advSrv.Publisher().NUuid(), nodeUuid);
  EXPECT_EQ(advSrv.Publisher().Scope(), scope);
  EXPECT_EQ(advSrv.Publisher().ReqTypeName(), reqType);
  EXPECT_EQ(advSrv.Publisher().RepTypeName(), repType);

  size_t msgLength = advSrv.Header().HeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + id.size() +
    sizeof(uint64_t) + pUuid.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t)  +
    sizeof(uint64_t) + advSrv.Publisher().ReqTypeName().size() +
    sizeof(uint64_t) + advSrv.Publisher().RepTypeName().size();
  EXPECT_EQ(advSrv.MsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseSrv setters.
  Header anotherHeader(version + 1, pUuid, AdvSrvType, 3);
  advSrv.SetHeader(anotherHeader);
  header = advSrv.Header();
  EXPECT_EQ(header.Version(), version + 1);
  EXPECT_EQ(header.PUuid(), anotherHeader.PUuid());
  EXPECT_EQ(header.Type(), AdvSrvType);
  EXPECT_EQ(header.Flags(), 3);
  size_t headerLength = sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags());
  EXPECT_EQ(static_cast<size_t>(header.HeaderLength()), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  id = "aSocketID";
  pUuid = "procUUID";
  nodeUuid = "nodeUUID2";
  scope = Scope_t::HOST;
  reqType = "Type1";
  repType = "Type2";
  advSrv.Publisher().SetTopic(topic);
  EXPECT_EQ(advSrv.Publisher().Topic(), topic);
  advSrv.Publisher().SetAddr(addr);
  EXPECT_EQ(advSrv.Publisher().Addr(), addr);
  advSrv.Publisher().SetSocketId(id);
  EXPECT_EQ(advSrv.Publisher().SocketId(), id);
  advSrv.Publisher().SetPUuid(pUuid);
  EXPECT_EQ(advSrv.Publisher().PUuid(), pUuid);
  advSrv.Publisher().SetNUuid(nodeUuid);
  EXPECT_EQ(advSrv.Publisher().NUuid(), nodeUuid);
  advSrv.Publisher().SetScope(scope);
  EXPECT_EQ(advSrv.Publisher().Scope(), scope);
  advSrv.Publisher().SetReqTypeName(reqType);
  EXPECT_EQ(advSrv.Publisher().ReqTypeName(), reqType);
  advSrv.Publisher().SetRepTypeName(repType);
  EXPECT_EQ(advSrv.Publisher().RepTypeName(), repType);

  // Check << operator
  std::ostringstream output;
  output << advSrv;
  std::string expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: ADV_SRV\n"
    "\tFlags: 3\n"
    "Publisher:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tProcess UUID: procUUID\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Host\n"
    "\tSocket ID: aSocketID\n"
    "\tRequest type: Type1\n"
    "\tResponse type: Type2\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of an ADV SRV message.
TEST(PacketTest, AdvertiseSrvIO)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;
  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string id = "socketId";
  std::string procUuid = "procUUID";
  std::string nodeUuid = "nodeUUID";
  Scope_t scope = Scope_t::HOST;
  std::string reqType = "StringMsg";
  std::string repType = "Int";

  // Try to pack an empty AdvertiseSrv.
  AdvertiseMessage<ServicePublisher> emptyMsg;
  std::vector<char> buffer(emptyMsg.MsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvertiseSrv (empty request type).
  Header otherHeader(version, pUuid, AdvType, 3);
  ServicePublisher publisherNoReqType(topic, addr, id, procUuid, nodeUuid,
    scope, "", repType);
  AdvertiseMessage<ServicePublisher> noReqMsg(otherHeader, publisherNoReqType);
  buffer.resize(noReqMsg.MsgLength());
  EXPECT_EQ(0u, noReqMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvertiseSrv (empty response type).
  ServicePublisher publisherNoRepType(topic, addr, id, procUuid, nodeUuid,
    scope, repType, "");
  AdvertiseMessage<ServicePublisher> noRepMsg(otherHeader, publisherNoRepType);
  buffer.resize(noRepMsg.MsgLength());
  EXPECT_EQ(0u, noRepMsg.Pack(&buffer[0]));

  // Pack an AdvertiseSrv.
  ServicePublisher publisher(topic, addr, id, procUuid, nodeUuid, scope,
    reqType, repType);
  AdvertiseMessage<ServicePublisher> advSrv(otherHeader, publisher);
  buffer.resize(advSrv.MsgLength());
  size_t bytes = advSrv.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advSrv.MsgLength());

  // Unpack an AdvertiseSrv.
  Header header;
  AdvertiseMessage<ServicePublisher> otherAdvSrv;
  size_t headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, static_cast<size_t>(header.HeaderLength()));
  otherAdvSrv.SetHeader(header);
  char *pBody = &buffer[0] + header.HeaderLength();
  size_t bodyBytes = otherAdvSrv.Unpack(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvSrv.Publisher().Topic(), advSrv.Publisher().Topic());
  EXPECT_EQ(otherAdvSrv.Publisher().Addr(), advSrv.Publisher().Addr());
  EXPECT_EQ(otherAdvSrv.Publisher().SocketId(), advSrv.Publisher().SocketId());
  EXPECT_EQ(otherAdvSrv.Publisher().NUuid(), advSrv.Publisher().NUuid());
  EXPECT_EQ(otherAdvSrv.Publisher().Scope(), advSrv.Publisher().Scope());
  EXPECT_EQ(otherAdvSrv.Publisher().ReqTypeName(),
    advSrv.Publisher().ReqTypeName());
  EXPECT_EQ(otherAdvSrv.Publisher().RepTypeName(),
    advSrv.Publisher().RepTypeName());
  EXPECT_EQ(otherAdvSrv.MsgLength(), advSrv.MsgLength());
  EXPECT_EQ(otherAdvSrv.MsgLength() -
            otherAdvSrv.Header().HeaderLength(), advSrv.MsgLength() -
            advSrv.Header().HeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvSrv.MsgLength() -
            otherAdvSrv.Header().HeaderLength());

  // Try to pack an AdvertiseSrv passing a NULL buffer.
  EXPECT_EQ(otherAdvSrv.Pack(nullptr), 0u);

  // Try to unpack an AdvertiseSrv passing a NULL buffer.
  EXPECT_EQ(otherAdvSrv.Unpack(nullptr), 0u);
}
