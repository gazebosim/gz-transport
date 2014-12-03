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
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the getters and setters.
TEST(PacketTest, BasicHeaderAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;
  transport::Header header(version, pUuid, transport::AdvType);

  // Check Header getters.
  EXPECT_EQ(version, header.Version());
  EXPECT_EQ(pUuid, header.PUuid());
  EXPECT_EQ(header.Type(), transport::AdvType);
  EXPECT_EQ(header.Flags(), 0);
  int headerLength = sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags());
  EXPECT_EQ(header.HeaderLength(), headerLength);

  // Check Header setters.
  pUuid = "Different-process-UUID-1";
  header.PUuid(pUuid);
  EXPECT_EQ(header.PUuid(), pUuid);
  header.Type(transport::SubType);
  EXPECT_EQ(header.Type(), transport::SubType);
  header.Flags(1);
  EXPECT_EQ(header.Flags(), 1);
  headerLength = sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags());
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
  transport::Header emptyHeader;
  std::vector<char> buffer(emptyHeader.HeaderLength());
  EXPECT_EQ(emptyHeader.Pack(&buffer[0]), 0u);

  // Pack a Header.
  transport::Header header(version, pUuid, transport::AdvSrvType, 2);

  buffer.resize(header.HeaderLength());
  int bytes = header.Pack(&buffer[0]);
  EXPECT_EQ(bytes, header.HeaderLength());

  // Unpack the Header.
  transport::Header otherHeader;
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

  transport::Header otherHeader(version, pUuid, transport::SubType, 3);

  std::string topic = "topic_test";
  transport::SubscriptionMsg subMsg(otherHeader, topic);

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
  transport::SubscriptionMsg emptyMsg;
  std::vector<char> buffer(emptyMsg.MsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Pack a SubscriptionMsg with an empty topic.
  transport::Header otherHeader(version, pUuid, transport::SubType, 3);
  transport::SubscriptionMsg incompleteMsg(otherHeader, "");
  buffer.resize(incompleteMsg.MsgLength());
  EXPECT_EQ(0u, incompleteMsg.Pack(&buffer[0]));

  // Pack a SubscriptionMsg.
  std::string topic = "topic_test";
  transport::SubscriptionMsg subMsg(otherHeader, topic);
  buffer.resize(subMsg.MsgLength());
  size_t bytes = subMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, subMsg.MsgLength());

  // Unpack a SubscriptionMsg.
  transport::Header header;
  transport::SubscriptionMsg otherSubMsg;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.HeaderLength());
  otherSubMsg.Header(header);
  char *pBody = &buffer[0] + header.HeaderLength();
  size_t bodyBytes = otherSubMsg.UnpackBody(pBody);

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
  EXPECT_EQ(otherSubMsg.UnpackBody(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV message.
TEST(PacketTest, BasicAdvertiseMsgAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);

  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::All;
  std::string typeName = "StringMsg";
  transport::AdvertiseMsg advMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, typeName);

  // Check AdvertiseMsg getters.
  transport::Header header = advMsg.Header();
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());
  EXPECT_EQ(header.HeaderLength(), otherHeader.HeaderLength());

  EXPECT_EQ(advMsg.Topic(), topic);
  EXPECT_EQ(advMsg.Address(), addr);
  EXPECT_EQ(advMsg.ControlAddress(), ctrl);
  EXPECT_EQ(advMsg.NodeUuid(), nodeUuid);
  EXPECT_EQ(advMsg.Scope(), scope);
  EXPECT_EQ(advMsg.MsgTypeName(), typeName);

  size_t msgLength = advMsg.Header().HeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + ctrl.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t) +
    sizeof(uint64_t) + advMsg.MsgTypeName().size();
  EXPECT_EQ(advMsg.MsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseMsg setters.
  transport::Header anotherHeader(version + 1, pUuid, transport::AdvSrvType, 3);
  advMsg.Header(anotherHeader);
  header = advMsg.Header();
  EXPECT_EQ(header.Version(), version + 1);
  EXPECT_EQ(header.PUuid(), anotherHeader.PUuid());
  EXPECT_EQ(header.Type(), transport::AdvSrvType);
  EXPECT_EQ(header.Flags(), 3);
  int headerLength = sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags());
  EXPECT_EQ(header.HeaderLength(), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  nodeUuid = "nodeUUID2";
  scope = transport::Scope::Host;
  typeName = "Int";
  advMsg.Topic(topic);
  EXPECT_EQ(advMsg.Topic(), topic);
  advMsg.Address(addr);
  EXPECT_EQ(advMsg.Address(), addr);
  advMsg.ControlAddress(ctrl);
  EXPECT_EQ(advMsg.ControlAddress(), ctrl);
  advMsg.NodeUuid(nodeUuid);
  EXPECT_EQ(advMsg.NodeUuid(), nodeUuid);
  advMsg.Scope(scope);
  EXPECT_EQ(advMsg.Scope(), scope);
  advMsg.MsgTypeName(typeName);
  EXPECT_EQ(advMsg.MsgTypeName(), typeName);

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
    "Body:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tControl address: inproc://control\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Host\n"
    "\tMessage type: Int\n";

  EXPECT_EQ(output.str(), expectedOutput);

  advMsg.Scope(transport::Scope::Process);
  output.str("");
  output << advMsg;
  expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: ADV_SRV\n"
    "\tFlags: 3\n"
    "Body:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tControl address: inproc://control\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Process\n"
    "\tMessage type: Int\n";

  EXPECT_EQ(output.str(), expectedOutput);

    // Check << operator
  advMsg.Scope(transport::Scope::All);
  output.str("");
  output << advMsg;
  expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tType: ADV_SRV\n"
    "\tFlags: 3\n"
    "Body:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tControl address: inproc://control\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: All\n"
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
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::Host;
  std::string typeName = "StringMsg";

  // Try to pack an empty AdvMsg.
  transport::AdvertiseMsg emptyMsg;
  std::vector<char> buffer(emptyMsg.MsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvMsg (empty topic).
  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);
  transport::AdvertiseMsg noTopicMsg(otherHeader, "", addr, ctrl, nodeUuid,
    scope, typeName);
  buffer.resize(noTopicMsg.MsgLength());
  EXPECT_EQ(0u, noTopicMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty address).
  transport::AdvertiseMsg noAddrMsg(otherHeader, topic, "", ctrl, nodeUuid,
    scope, typeName);
  buffer.resize(noAddrMsg.MsgLength());
  EXPECT_EQ(0u, noAddrMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty node UUID).
  transport::AdvertiseMsg noNodeUuidMsg(otherHeader, topic, addr, ctrl, "",
    scope, typeName);
  buffer.resize(noNodeUuidMsg.MsgLength());
  EXPECT_EQ(0u, noNodeUuidMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty message type name).
  transport::AdvertiseMsg noTypeMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, "");
  buffer.resize(noTypeMsg.MsgLength());
  EXPECT_EQ(0u, noTypeMsg.Pack(&buffer[0]));

  // Pack an AdvertiseMsg.
  transport::AdvertiseMsg advMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, typeName);
  buffer.resize(advMsg.MsgLength());
  size_t bytes = advMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advMsg.MsgLength());

  // Unpack an AdvertiseMsg.
  transport::Header header;
  transport::AdvertiseMsg otherAdvMsg;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.HeaderLength());
  otherAdvMsg.SetHeader(header);
  char *pBody = &buffer[0] + header.HeaderLength();
  size_t bodyBytes = otherAdvMsg.UnpackBody(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvMsg.Topic(), advMsg.Topic());
  EXPECT_EQ(otherAdvMsg.Address(), advMsg.Address());
  EXPECT_EQ(otherAdvMsg.ControlAddress(), advMsg.ControlAddress());
  EXPECT_EQ(otherAdvMsg.NodeUuid(), advMsg.NodeUuid());
  EXPECT_EQ(otherAdvMsg.Scope(), advMsg.Scope());
  EXPECT_EQ(otherAdvMsg.MsgTypeName(), advMsg.MsgTypeName());
  EXPECT_EQ(otherAdvMsg.MsgLength(), advMsg.MsgLength());
  EXPECT_EQ(otherAdvMsg.MsgLength() -
            otherAdvMsg.Header().HeaderLength(), advMsg.MsgLength() -
            advMsg.Header().HeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvMsg.MsgLength() -
            otherAdvMsg.Header().HeaderLength());

  // Try to pack an AdvertiseMsg passing a NULL buffer.
  EXPECT_EQ(otherAdvMsg.Pack(nullptr), 0u);

  // Try to unpack an AdvertiseMsg passing a NULL buffer.
  EXPECT_EQ(otherAdvMsg.UnpackBody(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV SRV message.
TEST(PacketTest, BasicAdvertiseSrvAPI)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);

  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::All;
  std::string reqType = "StringMsg";
  std::string repType = "Int";
  transport::AdvertiseSrv advSrv(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, reqType, repType);

  // Check AdvertiseSrv getters.
  transport::Header header = advSrv.Header();
  EXPECT_EQ(header.Version(), otherHeader.Version());
  EXPECT_EQ(header.PUuid(), otherHeader.PUuid());
  EXPECT_EQ(header.Type(), otherHeader.Type());
  EXPECT_EQ(header.Flags(), otherHeader.Flags());
  EXPECT_EQ(header.HeaderLength(), otherHeader.HeaderLength());

  EXPECT_EQ(advSrv.Topic(), topic);
  EXPECT_EQ(advSrv.Address(), addr);
  EXPECT_EQ(advSrv.ControlAddress(), ctrl);
  EXPECT_EQ(advSrv.NodeUuid(), nodeUuid);
  EXPECT_EQ(advSrv.Scope(), scope);
  EXPECT_EQ(advSrv.ReqTypeName(), reqType);
  EXPECT_EQ(advSrv.RepTypeName(), repType);

  size_t msgLength = advSrv.Header().HeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + ctrl.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t) +
    sizeof(uint64_t) + advSrv.ReqTypeName().size() +
    sizeof(uint64_t) + advSrv.RepTypeName().size();
  EXPECT_EQ(advSrv.MsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseSrv setters.
  transport::Header anotherHeader(version + 1, pUuid, transport::AdvSrvType, 3);
  advSrv.Header(anotherHeader);
  header = advSrv.Header();
  EXPECT_EQ(header.Version(), version + 1);
  EXPECT_EQ(header.PUuid(), anotherHeader.PUuid());
  EXPECT_EQ(header.Type(), transport::AdvSrvType);
  EXPECT_EQ(header.Flags(), 3);
  int headerLength = sizeof(header.Version()) +
    sizeof(uint64_t) + header.PUuid().size() +
    sizeof(header.Type()) + sizeof(header.Flags());
  EXPECT_EQ(header.HeaderLength(), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  nodeUuid = "nodeUUID2";
  scope = transport::Scope::Host;
  reqType = "Type1";
  repType = "Type2";
  advSrv.Topic(topic);
  EXPECT_EQ(advSrv.Topic(), topic);
  advSrv.Address(addr);
  EXPECT_EQ(advSrv.Address(), addr);
  advSrv.ControlAddress(ctrl);
  EXPECT_EQ(advSrv.ControlAddress(), ctrl);
  advSrv.NodeUuid(nodeUuid);
  EXPECT_EQ(advSrv.NodeUuid(), nodeUuid);
  advSrv.Scope(scope);
  EXPECT_EQ(advSrv.Scope(), scope);
  advSrv.ReqTypeName(reqType);
  EXPECT_EQ(advSrv.ReqTypeName(), reqType);
  advSrv.RepTypeName(repType);
  EXPECT_EQ(advSrv.RepTypeName(), repType);

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
    "Body:\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tAddress: inproc://local\n"
    "\tControl address: inproc://control\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Host\n"
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
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::Host;
  std::string reqType = "StringMsg";
  std::string repType = "Int";

  // Try to pack an empty AdvertiseSrv.
  transport::AdvertiseSrv emptyMsg;
  std::vector<char> buffer(emptyMsg.MsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvertiseSrv (empty request type).
  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);
  transport::AdvertiseSrv noReqMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, "", repType);
  buffer.resize(noReqMsg.MsgLength());
  EXPECT_EQ(0u, noReqMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvertiseSrv (empty response type).
  transport::AdvertiseSrv noRepMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, reqType, "");
  buffer.resize(noRepMsg.MsgLength());
  EXPECT_EQ(0u, noRepMsg.Pack(&buffer[0]));

  // Pack an AdvertiseSrv.
  transport::AdvertiseSrv advSrv(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, reqType, repType);
  buffer.resize(advSrv.MsgLength());
  size_t bytes = advSrv.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advSrv.MsgLength());

  // Unpack an AdvertiseSrv.
  transport::Header header;
  transport::AdvertiseSrv otherAdvSrv;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.HeaderLength());
  otherAdvSrv.Header(header);
  char *pBody = &buffer[0] + header.HeaderLength();
  size_t bodyBytes = otherAdvSrv.UnpackBody(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvSrv.Topic(), advSrv.Topic());
  EXPECT_EQ(otherAdvSrv.Address(), advSrv.Address());
  EXPECT_EQ(otherAdvSrv.ControlAddress(), advSrv.ControlAddress());
  EXPECT_EQ(otherAdvSrv.NodeUuid(), advSrv.NodeUuid());
  EXPECT_EQ(otherAdvSrv.Scope(), advSrv.Scope());
  EXPECT_EQ(otherAdvSrv.ReqTypeName(), advSrv.ReqTypeName());
  EXPECT_EQ(otherAdvSrv.RepTypeName(), advSrv.RepTypeName());
  EXPECT_EQ(otherAdvSrv.MsgLength(), advSrv.MsgLength());
  EXPECT_EQ(otherAdvSrv.MsgLength() -
            otherAdvSrv.Header().HeaderLength(), advSrv.MsgLength() -
            advSrv.Header().HeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvSrv.MsgLength() -
            otherAdvSrv.Header().HeaderLength());

  // Try to pack an AdvertiseSrv passing a NULL buffer.
  EXPECT_EQ(otherAdvSrv.Pack(nullptr), 0u);

  // Try to unpack an AdvertiseSrv passing a NULL buffer.
  EXPECT_EQ(otherAdvSrv.UnpackBody(nullptr), 0u);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
