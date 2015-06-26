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

  // Check Header setters.
  pUuid = "Different-process-UUID-1";
  header.PUuid(pUuid);
  EXPECT_EQ(header.PUuid(), pUuid);
  header.Type(SubType);
  EXPECT_EQ(header.Type(), SubType);
  header.Flags(1);
  EXPECT_EQ(header.Flags(), 1);

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

/// \brief Check the serialization and unserialization of a header.
TEST(PacketTest, HeaderIO)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  // Try to pack an empty header.
  Header emptyHeader;
  std::vector<char> buffer;
  EXPECT_FALSE(emptyHeader.Pack(buffer));

  // Pack a Header.
  Header header(version, pUuid, AdvSrvType, 2);

  EXPECT_TRUE(header.Pack(buffer));

  // Unpack the Header.
  Header otherHeader;
  EXPECT_TRUE(otherHeader.Unpack(buffer));

  // Check that after Pack() and Unpack() the Header remains the same.
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());

  // Try to unpack a header passing an empty buffer.
  EXPECT_FALSE(otherHeader.Unpack(std::vector<char>()));
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

  // Check Sub setters.
  topic = "a_new_topic_test";
  subMsg.Topic(topic);
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
  std::vector<char> buffer;
  EXPECT_FALSE(emptyMsg.Pack(buffer));

  // Pack a SubscriptionMsg with an empty topic.
  Header otherHeader(version, pUuid, SubType, 3);
  SubscriptionMsg incompleteMsg(otherHeader, "");
  EXPECT_FALSE(incompleteMsg.Pack(buffer));

  // Pack a SubscriptionMsg.
  std::string topic = "topic_test";
  SubscriptionMsg subMsg(otherHeader, topic);
  EXPECT_TRUE(subMsg.Pack(buffer));

  // Unpack a SubscriptionMsg.
  SubscriptionMsg otherSubMsg;
  EXPECT_TRUE(otherSubMsg.Unpack(buffer));

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherSubMsg.Topic(), subMsg.Topic());
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
  Scope_t scope = Scope_t::All;
  std::string typeName = "StringMsg";
  MessagePublisher pub(topic, addr, ctrl, procUuid, nodeUuid, scope, typeName);
  AdvertiseMessage<MessagePublisher> advMsg(otherHeader, pub);

  // Check AdvertiseMsg getters.
  Header header = advMsg.GetHeader();
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());

  EXPECT_EQ(advMsg.GetPublisher().Topic(), topic);
  EXPECT_EQ(advMsg.GetPublisher().Addr(), addr);
  EXPECT_EQ(advMsg.GetPublisher().Ctrl(), ctrl);
  EXPECT_EQ(advMsg.GetPublisher().NUuid(), nodeUuid);
  EXPECT_EQ(advMsg.GetPublisher().Scope(), scope);
  EXPECT_EQ(advMsg.GetPublisher().MsgTypeName(), typeName);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseMsg setters.
  Header anotherHeader(version + 1, pUuid, AdvSrvType, 3);
  EXPECT_EQ(anotherHeader.Version(), version + 1);
  advMsg.SetHeader(anotherHeader);
  EXPECT_EQ(advMsg.GetHeader().Version(), version + 1);
  header = advMsg.GetHeader();
  EXPECT_EQ(header.Version(), version + 1);
  EXPECT_EQ(header.PUuid(), anotherHeader.PUuid());
  EXPECT_EQ(header.Type(), AdvSrvType);
  EXPECT_EQ(header.Flags(), 3);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  procUuid = "procUUID";
  nodeUuid = "nodeUUID2";
  scope = Scope_t::Host;
  typeName = "Int";
  advMsg.GetPublisher().Topic(topic);
  EXPECT_EQ(advMsg.GetPublisher().Topic(), topic);
  advMsg.GetPublisher().Addr(addr);
  EXPECT_EQ(advMsg.GetPublisher().Addr(), addr);
  advMsg.GetPublisher().PUuid(procUuid);
  EXPECT_EQ(advMsg.GetPublisher().PUuid(), procUuid);
  advMsg.GetPublisher().Ctrl(ctrl);
  EXPECT_EQ(advMsg.GetPublisher().Ctrl(), ctrl);
  advMsg.GetPublisher().NUuid(nodeUuid);
  EXPECT_EQ(advMsg.GetPublisher().NUuid(), nodeUuid);
  advMsg.GetPublisher().Scope(scope);
  EXPECT_EQ(advMsg.GetPublisher().Scope(), scope);
  advMsg.GetPublisher().MsgTypeName(typeName);
  EXPECT_EQ(advMsg.GetPublisher().MsgTypeName(), typeName);

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

  advMsg.GetPublisher().Scope(Scope_t::Process);
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
  advMsg.GetPublisher().Scope(Scope_t::All);
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
  Scope_t scope = Scope_t::Host;
  std::string typeName = "StringMsg";

  // Try to pack an empty AdvMsg.
  AdvertiseMessage<MessagePublisher> emptyMsg;
  std::vector<char> buffer;
  EXPECT_FALSE(emptyMsg.Pack(buffer));

  // Try to pack an incomplete AdvMsg (empty topic).
  Header otherHeader(version, pUuid, AdvType, 3);
  MessagePublisher publisherNoTopic("", addr, ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noTopicMsg(otherHeader, publisherNoTopic);
  EXPECT_FALSE(noTopicMsg.Pack(buffer));

  // Try to pack an incomplete AdvMsg (empty address).
  MessagePublisher publisherNoAddr(topic, "", ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noAddrMsg(otherHeader, publisherNoAddr);
  EXPECT_FALSE(noAddrMsg.Pack(buffer));

  // Try to pack an incomplete AdvMsg (empty node UUID).
  MessagePublisher publisherNoNUuid(topic, addr, ctrl, procUuid, "", scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noNodeUuidMsg(otherHeader,
    publisherNoNUuid);
  EXPECT_FALSE(noNodeUuidMsg.Pack(buffer));

  // Try to pack an incomplete AdvMsg (empty message type name).
  MessagePublisher publisherNoMsgType(topic, addr, ctrl, procUuid, nodeUuid,
    scope, "");
  AdvertiseMessage<MessagePublisher> noTypeMsg(otherHeader, publisherNoMsgType);
  EXPECT_FALSE(noTypeMsg.Pack(buffer));

  // Pack an AdvertiseMsg.
  MessagePublisher publisher(topic, addr, ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> advMsg(otherHeader, publisher);
  EXPECT_TRUE(advMsg.Pack(buffer));

  // Unpack an AdvertiseMsg.
  AdvertiseMessage<MessagePublisher> otherAdvMsg;
  EXPECT_TRUE(otherAdvMsg.Unpack(buffer));

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvMsg.GetPublisher().Topic(), advMsg.GetPublisher().Topic());
  EXPECT_EQ(otherAdvMsg.GetPublisher().Addr(), advMsg.GetPublisher().Addr());
  EXPECT_EQ(otherAdvMsg.GetPublisher().Ctrl(), advMsg.GetPublisher().Ctrl());
  EXPECT_EQ(otherAdvMsg.GetPublisher().NUuid(), advMsg.GetPublisher().NUuid());
  EXPECT_EQ(otherAdvMsg.GetPublisher().Scope(), advMsg.GetPublisher().Scope());
  EXPECT_EQ(otherAdvMsg.GetPublisher().MsgTypeName(),
    advMsg.GetPublisher().MsgTypeName());
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
  Scope_t scope = Scope_t::All;
  std::string reqType = "StringMsg";
  std::string repType = "Int";

  ServicePublisher publisher(topic, addr, id, pUuid, nodeUuid, scope, reqType,
    repType);
  AdvertiseMessage<ServicePublisher> advSrv(otherHeader, publisher);

  // Check AdvertiseSrv getters.
  Header header = advSrv.GetHeader();
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());

  EXPECT_EQ(advSrv.GetPublisher().Topic(), topic);
  EXPECT_EQ(advSrv.GetPublisher().Addr(), addr);
  EXPECT_EQ(advSrv.GetPublisher().SocketId(), id);
  EXPECT_EQ(advSrv.GetPublisher().PUuid(), pUuid);
  EXPECT_EQ(advSrv.GetPublisher().NUuid(), nodeUuid);
  EXPECT_EQ(advSrv.GetPublisher().Scope(), scope);
  EXPECT_EQ(advSrv.GetPublisher().ReqTypeName(), reqType);
  EXPECT_EQ(advSrv.GetPublisher().RepTypeName(), repType);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseSrv setters.
  Header anotherHeader(version + 1, pUuid, AdvSrvType, 3);
  advSrv.SetHeader(anotherHeader);
  header = advSrv.GetHeader();
  EXPECT_EQ(header.Version(), version + 1);
  EXPECT_EQ(header.PUuid(), anotherHeader.PUuid());
  EXPECT_EQ(header.Type(), AdvSrvType);
  EXPECT_EQ(header.Flags(), 3);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  id = "aSocketID";
  pUuid = "procUUID";
  nodeUuid = "nodeUUID2";
  scope = Scope_t::Host;
  reqType = "Type1";
  repType = "Type2";
  advSrv.GetPublisher().Topic(topic);
  EXPECT_EQ(advSrv.GetPublisher().Topic(), topic);
  advSrv.GetPublisher().Addr(addr);
  EXPECT_EQ(advSrv.GetPublisher().Addr(), addr);
  advSrv.GetPublisher().SocketId(id);
  EXPECT_EQ(advSrv.GetPublisher().SocketId(), id);
  advSrv.GetPublisher().PUuid(pUuid);
  EXPECT_EQ(advSrv.GetPublisher().PUuid(), pUuid);
  advSrv.GetPublisher().NUuid(nodeUuid);
  EXPECT_EQ(advSrv.GetPublisher().NUuid(), nodeUuid);
  advSrv.GetPublisher().Scope(scope);
  EXPECT_EQ(advSrv.GetPublisher().Scope(), scope);
  advSrv.GetPublisher().ReqTypeName(reqType);
  EXPECT_EQ(advSrv.GetPublisher().ReqTypeName(), reqType);
  advSrv.GetPublisher().RepTypeName(repType);
  EXPECT_EQ(advSrv.GetPublisher().RepTypeName(), repType);

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
  Scope_t scope = Scope_t::Host;
  std::string reqType = "StringMsg";
  std::string repType = "Int";

  // Try to pack an empty AdvertiseSrv.
  AdvertiseMessage<ServicePublisher> emptyMsg;
  std::vector<char> buffer;
  EXPECT_FALSE(emptyMsg.Pack(buffer));

  // Try to pack an incomplete AdvertiseSrv (empty request type).
  Header otherHeader(version, pUuid, AdvType, 3);
  ServicePublisher publisherNoReqType(topic, addr, id, procUuid, nodeUuid,
    scope, "", repType);
  AdvertiseMessage<ServicePublisher> noReqMsg(otherHeader, publisherNoReqType);
  EXPECT_FALSE(noReqMsg.Pack(buffer));

  // Try to pack an incomplete AdvertiseSrv (empty response type).
  ServicePublisher publisherNoRepType(topic, addr, id, procUuid, nodeUuid,
    scope, repType, "");
  AdvertiseMessage<ServicePublisher> noRepMsg(otherHeader, publisherNoRepType);
  EXPECT_FALSE(noRepMsg.Pack(buffer));

  // Pack an AdvertiseSrv.
  ServicePublisher publisher(topic, addr, id, procUuid, nodeUuid, scope,
    reqType, repType);
  AdvertiseMessage<ServicePublisher> advSrv(otherHeader, publisher);
  EXPECT_TRUE(advSrv.Pack(buffer));

  // Unpack an AdvertiseSrv.
  AdvertiseMessage<ServicePublisher> otherAdvSrv;
  EXPECT_TRUE(otherAdvSrv.Unpack(buffer));

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvSrv.GetPublisher().Topic(), advSrv.GetPublisher().Topic());
  EXPECT_EQ(otherAdvSrv.GetPublisher().Addr(), advSrv.GetPublisher().Addr());
  EXPECT_EQ(otherAdvSrv.GetPublisher().SocketId(),
    advSrv.GetPublisher().SocketId());
  EXPECT_EQ(otherAdvSrv.GetPublisher().NUuid(), advSrv.GetPublisher().NUuid());
  EXPECT_EQ(otherAdvSrv.GetPublisher().Scope(), advSrv.GetPublisher().Scope());
  EXPECT_EQ(otherAdvSrv.GetPublisher().ReqTypeName(),
    advSrv.GetPublisher().ReqTypeName());
  EXPECT_EQ(otherAdvSrv.GetPublisher().RepTypeName(),
    advSrv.GetPublisher().RepTypeName());
}
