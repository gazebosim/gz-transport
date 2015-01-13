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
  EXPECT_EQ(version, header.GetVersion());
  EXPECT_EQ(pUuid, header.GetPUuid());
  EXPECT_EQ(header.GetType(), AdvType);
  EXPECT_EQ(header.GetFlags(), 0);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  // Check Header setters.
  pUuid = "Different-process-UUID-1";
  header.SetPUuid(pUuid);
  EXPECT_EQ(header.GetPUuid(), pUuid);
  header.SetType(SubType);
  EXPECT_EQ(header.GetType(), SubType);
  header.SetFlags(1);
  EXPECT_EQ(header.GetFlags(), 1);
  headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

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
  std::vector<char> buffer(emptyHeader.GetHeaderLength());
  EXPECT_EQ(emptyHeader.Pack(&buffer[0]), 0u);

  // Pack a Header.
  Header header(version, pUuid, AdvSrvType, 2);

  buffer.resize(header.GetHeaderLength());
  int bytes = header.Pack(&buffer[0]);
  EXPECT_EQ(bytes, header.GetHeaderLength());

  // Unpack the Header.
  Header otherHeader;
  otherHeader.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Header remains the same.
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

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
  EXPECT_EQ(subMsg.GetTopic(), topic);

  size_t msgLength = subMsg.GetHeader().GetHeaderLength() +
    sizeof(uint64_t) + topic.size();
  EXPECT_EQ(subMsg.GetMsgLength(), msgLength);

  // Check Sub setters.
  topic = "a_new_topic_test";
  subMsg.SetTopic(topic);
  EXPECT_EQ(subMsg.GetTopic(), topic);

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
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Pack a SubscriptionMsg with an empty topic.
  Header otherHeader(version, pUuid, SubType, 3);
  SubscriptionMsg incompleteMsg(otherHeader, "");
  buffer.resize(incompleteMsg.GetMsgLength());
  EXPECT_EQ(0u, incompleteMsg.Pack(&buffer[0]));

  // Pack a SubscriptionMsg.
  std::string topic = "topic_test";
  SubscriptionMsg subMsg(otherHeader, topic);
  buffer.resize(subMsg.GetMsgLength());
  size_t bytes = subMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, subMsg.GetMsgLength());

  // Unpack a SubscriptionMsg.
  Header header;
  SubscriptionMsg otherSubMsg;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherSubMsg.SetHeader(header);
  char *pBody = &buffer[0] + header.GetHeaderLength();
  size_t bodyBytes = otherSubMsg.Unpack(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherSubMsg.GetTopic(), subMsg.GetTopic());
  EXPECT_EQ(otherSubMsg.GetMsgLength() -
            otherSubMsg.GetHeader().GetHeaderLength(), subMsg.GetMsgLength() -
            subMsg.GetHeader().GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherSubMsg.GetMsgLength() -
            otherSubMsg.GetHeader().GetHeaderLength());

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
  Scope_t scope = Scope_t::All;
  std::string typeName = "StringMsg";
  MessagePublisher pub(topic, addr, ctrl, procUuid, nodeUuid, scope, typeName);
  AdvertiseMessage<MessagePublisher> advMsg(otherHeader, pub);

  // Check AdvertiseMsg getters.
  Header header = advMsg.header;
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

  EXPECT_EQ(advMsg.publisher.Topic(), topic);
  EXPECT_EQ(advMsg.publisher.Addr(), addr);
  EXPECT_EQ(advMsg.publisher.Ctrl(), ctrl);
  EXPECT_EQ(advMsg.publisher.NUuid(), nodeUuid);
  EXPECT_EQ(advMsg.publisher.Scope(), scope);
  EXPECT_EQ(advMsg.publisher.MsgTypeName(), typeName);

  size_t msgLength = advMsg.header.GetHeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + ctrl.size() +
    sizeof(uint64_t) + procUuid.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t)  +
    sizeof(uint64_t) + typeName.size();
  EXPECT_EQ(advMsg.GetMsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseMsg setters.
  Header anotherHeader(version + 1, pUuid, AdvSrvType, 3);
  advMsg.header = anotherHeader;
  header = advMsg.header;
  EXPECT_EQ(header.GetVersion(), version + 1);
  EXPECT_EQ(header.GetPUuid(), anotherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), AdvSrvType);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  procUuid = "procUUID";
  nodeUuid = "nodeUUID2";
  scope = Scope_t::Host;
  typeName = "Int";
  advMsg.publisher.Topic(topic);
  EXPECT_EQ(advMsg.publisher.Topic(), topic);
  advMsg.publisher.Addr(addr);
  EXPECT_EQ(advMsg.publisher.Addr(), addr);
  advMsg.publisher.PUuid(procUuid);
  EXPECT_EQ(advMsg.publisher.PUuid(), procUuid);
  advMsg.publisher.Ctrl(ctrl);
  EXPECT_EQ(advMsg.publisher.Ctrl(), ctrl);
  advMsg.publisher.NUuid(nodeUuid);
  EXPECT_EQ(advMsg.publisher.NUuid(), nodeUuid);
  advMsg.publisher.Scope(scope);
  EXPECT_EQ(advMsg.publisher.Scope(), scope);
  advMsg.publisher.MsgTypeName(typeName);
  EXPECT_EQ(advMsg.publisher.MsgTypeName(), typeName);

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

  advMsg.publisher.Scope(Scope_t::Process);
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
  advMsg.publisher.Scope(Scope_t::All);
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
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvMsg (empty topic).
  Header otherHeader(version, pUuid, AdvType, 3);
  MessagePublisher publisherNoTopic("", addr, ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noTopicMsg(otherHeader, publisherNoTopic);
  buffer.resize(noTopicMsg.GetMsgLength());
  EXPECT_EQ(0u, noTopicMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty address).
  MessagePublisher publisherNoAddr(topic, "", ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noAddrMsg(otherHeader, publisherNoAddr);
  buffer.resize(noAddrMsg.GetMsgLength());
  EXPECT_EQ(0u, noAddrMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty node UUID).
  MessagePublisher publisherNoNUuid(topic, addr, ctrl, procUuid, "", scope,
    typeName);
  AdvertiseMessage<MessagePublisher> noNodeUuidMsg(otherHeader,
    publisherNoNUuid);
  buffer.resize(noNodeUuidMsg.GetMsgLength());
  EXPECT_EQ(0u, noNodeUuidMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty message type name).
  MessagePublisher publisherNoMsgType(topic, addr, ctrl, procUuid, nodeUuid,
    scope, "");
  AdvertiseMessage<MessagePublisher> noTypeMsg(otherHeader, publisherNoMsgType);
  buffer.resize(noTypeMsg.GetMsgLength());
  EXPECT_EQ(0u, noTypeMsg.Pack(&buffer[0]));

  // Pack an AdvertiseMsg.
  MessagePublisher publisher(topic, addr, ctrl, procUuid, nodeUuid, scope,
    typeName);
  AdvertiseMessage<MessagePublisher> advMsg(otherHeader, publisher);
  buffer.resize(advMsg.GetMsgLength());
  size_t bytes = advMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advMsg.GetMsgLength());

  // Unpack an AdvertiseMsg.
  Header header;
  AdvertiseMessage<MessagePublisher> otherAdvMsg;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherAdvMsg.header = header;
  char *pBody = &buffer[0] + header.GetHeaderLength();
  size_t bodyBytes = otherAdvMsg.Unpack(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvMsg.publisher.Topic(), advMsg.publisher.Topic());
  EXPECT_EQ(otherAdvMsg.publisher.Addr(), advMsg.publisher.Addr());
  EXPECT_EQ(otherAdvMsg.publisher.Ctrl(), advMsg.publisher.Ctrl());
  EXPECT_EQ(otherAdvMsg.publisher.NUuid(), advMsg.publisher.NUuid());
  EXPECT_EQ(otherAdvMsg.publisher.Scope(), advMsg.publisher.Scope());
  EXPECT_EQ(otherAdvMsg.publisher.MsgTypeName(),
    advMsg.publisher.MsgTypeName());
  EXPECT_EQ(otherAdvMsg.GetMsgLength(), advMsg.GetMsgLength());
  EXPECT_EQ(otherAdvMsg.GetMsgLength() -
            otherAdvMsg.header.GetHeaderLength(), advMsg.GetMsgLength() -
            advMsg.header.GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvMsg.GetMsgLength() -
            otherAdvMsg.header.GetHeaderLength());

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
  Scope_t scope = Scope_t::All;
  std::string reqType = "StringMsg";
  std::string repType = "Int";

  ServicePublisher publisher(topic, addr, id, pUuid, nodeUuid, scope, reqType,
    repType);
  AdvertiseMessage<ServicePublisher> advSrv(otherHeader, publisher);

  // Check AdvertiseSrv getters.
  Header header = advSrv.header;
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

  EXPECT_EQ(advSrv.publisher.Topic(), topic);
  EXPECT_EQ(advSrv.publisher.Addr(), addr);
  EXPECT_EQ(advSrv.publisher.SocketId(), id);
  EXPECT_EQ(advSrv.publisher.PUuid(), pUuid);
  EXPECT_EQ(advSrv.publisher.NUuid(), nodeUuid);
  EXPECT_EQ(advSrv.publisher.Scope(), scope);
  EXPECT_EQ(advSrv.publisher.ReqTypeName(), reqType);
  EXPECT_EQ(advSrv.publisher.RepTypeName(), repType);

  size_t msgLength = advSrv.header.GetHeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + id.size() +
    sizeof(uint64_t) + pUuid.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t)  +
    sizeof(uint64_t) + advSrv.publisher.ReqTypeName().size() +
    sizeof(uint64_t) + advSrv.publisher.RepTypeName().size();
  EXPECT_EQ(advSrv.GetMsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseSrv setters.
  Header anotherHeader(version + 1, pUuid, AdvSrvType, 3);
  advSrv.header = anotherHeader;
  header = advSrv.header;
  EXPECT_EQ(header.GetVersion(), version + 1);
  EXPECT_EQ(header.GetPUuid(), anotherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), AdvSrvType);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  id = "aSocketID";
  pUuid = "procUUID";
  nodeUuid = "nodeUUID2";
  scope = Scope_t::Host;
  reqType = "Type1";
  repType = "Type2";
  advSrv.publisher.Topic(topic);
  EXPECT_EQ(advSrv.publisher.Topic(), topic);
  advSrv.publisher.Addr(addr);
  EXPECT_EQ(advSrv.publisher.Addr(), addr);
  advSrv.publisher.SocketId(id);
  EXPECT_EQ(advSrv.publisher.SocketId(), id);
  advSrv.publisher.PUuid(pUuid);
  EXPECT_EQ(advSrv.publisher.PUuid(), pUuid);
  advSrv.publisher.NUuid(nodeUuid);
  EXPECT_EQ(advSrv.publisher.NUuid(), nodeUuid);
  advSrv.publisher.Scope(scope);
  EXPECT_EQ(advSrv.publisher.Scope(), scope);
  advSrv.publisher.ReqTypeName(reqType);
  EXPECT_EQ(advSrv.publisher.ReqTypeName(), reqType);
  advSrv.publisher.RepTypeName(repType);
  EXPECT_EQ(advSrv.publisher.RepTypeName(), repType);

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
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvertiseSrv (empty request type).
  Header otherHeader(version, pUuid, AdvType, 3);
  ServicePublisher publisherNoReqType(topic, addr, id, procUuid, nodeUuid,
    scope, "", repType);
  AdvertiseMessage<ServicePublisher> noReqMsg(otherHeader, publisherNoReqType);
  buffer.resize(noReqMsg.GetMsgLength());
  EXPECT_EQ(0u, noReqMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvertiseSrv (empty response type).
  ServicePublisher publisherNoRepType(topic, addr, id, procUuid, nodeUuid,
    scope, repType, "");
  AdvertiseMessage<ServicePublisher> noRepMsg(otherHeader, publisherNoRepType);
  buffer.resize(noRepMsg.GetMsgLength());
  EXPECT_EQ(0u, noRepMsg.Pack(&buffer[0]));

  // Pack an AdvertiseSrv.
  ServicePublisher publisher(topic, addr, id, procUuid, nodeUuid, scope,
    reqType, repType);
  AdvertiseMessage<ServicePublisher> advSrv(otherHeader, publisher);
  buffer.resize(advSrv.GetMsgLength());
  size_t bytes = advSrv.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advSrv.GetMsgLength());

  // Unpack an AdvertiseSrv.
  Header header;
  AdvertiseMessage<ServicePublisher> otherAdvSrv;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherAdvSrv.header = header;
  char *pBody = &buffer[0] + header.GetHeaderLength();
  size_t bodyBytes = otherAdvSrv.Unpack(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvSrv.publisher.Topic(), advSrv.publisher.Topic());
  EXPECT_EQ(otherAdvSrv.publisher.Addr(), advSrv.publisher.Addr());
  EXPECT_EQ(otherAdvSrv.publisher.SocketId(), advSrv.publisher.SocketId());
  EXPECT_EQ(otherAdvSrv.publisher.NUuid(), advSrv.publisher.NUuid());
  EXPECT_EQ(otherAdvSrv.publisher.Scope(), advSrv.publisher.Scope());
  EXPECT_EQ(otherAdvSrv.publisher.ReqTypeName(),
    advSrv.publisher.ReqTypeName());
  EXPECT_EQ(otherAdvSrv.publisher.RepTypeName(),
    advSrv.publisher.RepTypeName());
  EXPECT_EQ(otherAdvSrv.GetMsgLength(), advSrv.GetMsgLength());
  EXPECT_EQ(otherAdvSrv.GetMsgLength() -
            otherAdvSrv.header.GetHeaderLength(), advSrv.GetMsgLength() -
            advSrv.header.GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvSrv.GetMsgLength() -
            otherAdvSrv.header.GetHeaderLength());

  // Try to pack an AdvertiseSrv passing a NULL buffer.
  EXPECT_EQ(otherAdvSrv.Pack(nullptr), 0u);

  // Try to unpack an AdvertiseSrv passing a NULL buffer.
  EXPECT_EQ(otherAdvSrv.Unpack(nullptr), 0u);
}
