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
  EXPECT_EQ(version, header.GetVersion());
  EXPECT_EQ(pUuid, header.GetPUuid());
  EXPECT_EQ(header.GetType(), transport::AdvType);
  EXPECT_EQ(header.GetFlags(), 0);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  // Check Header setters.
  pUuid = "Different-process-UUID-1";
  header.SetPUuid(pUuid);
  EXPECT_EQ(header.GetPUuid(), pUuid);
  header.SetType(transport::SubType);
  EXPECT_EQ(header.GetType(), transport::SubType);
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
  transport::Header emptyHeader;
  std::vector<char> buffer(emptyHeader.GetHeaderLength());
  EXPECT_EQ(emptyHeader.Pack(&buffer[0]), 0u);

  // Pack a Header.
  transport::Header header(version, pUuid, transport::AdvSrvType, 2);

  buffer.resize(header.GetHeaderLength());
  int bytes = header.Pack(&buffer[0]);
  EXPECT_EQ(bytes, header.GetHeaderLength());

  // Unpack the Header.
  transport::Header otherHeader;
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

  transport::Header otherHeader(version, pUuid, transport::SubType, 3);

  std::string topic = "topic_test";
  transport::SubscriptionMsg subMsg(otherHeader, topic);

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
  transport::SubscriptionMsg emptyMsg;
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Pack a SubscriptionMsg with an empty topic.
  transport::Header otherHeader(version, pUuid, transport::SubType, 3);
  transport::SubscriptionMsg incompleteMsg(otherHeader, "");
  buffer.resize(incompleteMsg.GetMsgLength());
  EXPECT_EQ(0u, incompleteMsg.Pack(&buffer[0]));

  // Pack a SubscriptionMsg.
  std::string topic = "topic_test";
  transport::SubscriptionMsg subMsg(otherHeader, topic);
  buffer.resize(subMsg.GetMsgLength());
  size_t bytes = subMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, subMsg.GetMsgLength());

  // Unpack a SubscriptionMsg.
  transport::Header header;
  transport::SubscriptionMsg otherSubMsg;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherSubMsg.SetHeader(header);
  char *pBody = &buffer[0] + header.GetHeaderLength();
  size_t bodyBytes = otherSubMsg.UnpackBody(pBody);

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
  std::string subscribers = "\tsubscriber1:12345\n\tsubscriber2:23456\n";
  transport::AdvertiseMsg advMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, typeName, subscribers);

  // Check AdvertiseMsg getters.
  transport::Header header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

  EXPECT_EQ(advMsg.GetTopic(), topic);
  EXPECT_EQ(advMsg.GetAddress(), addr);
  EXPECT_EQ(advMsg.GetControlAddress(), ctrl);
  EXPECT_EQ(advMsg.GetNodeUuid(), nodeUuid);
  EXPECT_EQ(advMsg.GetScope(), scope);
  EXPECT_EQ(advMsg.GetMsgTypeName(), typeName);

  size_t msgLength = advMsg.GetHeader().GetHeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + ctrl.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t) +
    sizeof(uint64_t) + advMsg.GetMsgTypeName().size() +
    sizeof(uint64_t) + advMsg.GetSubscribers().size();
  EXPECT_EQ(advMsg.GetMsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseMsg setters.
  transport::Header anotherHeader(version + 1, pUuid, transport::AdvSrvType, 3);
  advMsg.SetHeader(anotherHeader);
  header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), version + 1);
  EXPECT_EQ(header.GetPUuid(), anotherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), transport::AdvSrvType);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  nodeUuid = "nodeUUID2";
  scope = transport::Scope::Host;
  typeName = "Int";
  advMsg.SetTopic(topic);
  EXPECT_EQ(advMsg.GetTopic(), topic);
  advMsg.SetAddress(addr);
  EXPECT_EQ(advMsg.GetAddress(), addr);
  advMsg.SetControlAddress(ctrl);
  EXPECT_EQ(advMsg.GetControlAddress(), ctrl);
  advMsg.SetNodeUuid(nodeUuid);
  EXPECT_EQ(advMsg.GetNodeUuid(), nodeUuid);
  advMsg.SetScope(scope);
  EXPECT_EQ(advMsg.GetScope(), scope);
  advMsg.SetMsgTypeName(typeName);
  EXPECT_EQ(advMsg.GetMsgTypeName(), typeName);

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
    "\tMessage type: Int\n"
    "\tSubscribers:\n\tsubscriber1:12345\n\tsubscriber2:23456\n\n";

  EXPECT_EQ(output.str(), expectedOutput);

  advMsg.SetScope(transport::Scope::Process);
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
    "\tMessage type: Int\n"
    "\tSubscribers:\n\tsubscriber1:12345\n\tsubscriber2:23456\n\n";

  EXPECT_EQ(output.str(), expectedOutput);

    // Check << operator
  advMsg.SetScope(transport::Scope::All);
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
    "\tMessage type: Int\n"
    "\tSubscribers:\n\tsubscriber1:12345\n\tsubscriber2:23456\n\n";

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
  std::string subscribers = "subscriber1:12345\nsubscriber2:23456\n";

  // Try to pack an empty AdvMsg.
  transport::AdvertiseMsg emptyMsg;
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvMsg (empty topic).
  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);
  transport::AdvertiseMsg noTopicMsg(otherHeader, "", addr, ctrl, nodeUuid,
    scope, typeName, subscribers);
  buffer.resize(noTopicMsg.GetMsgLength());
  EXPECT_EQ(0u, noTopicMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty address).
  transport::AdvertiseMsg noAddrMsg(otherHeader, topic, "", ctrl, nodeUuid,
    scope, typeName, subscribers);
  buffer.resize(noAddrMsg.GetMsgLength());
  EXPECT_EQ(0u, noAddrMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty node UUID).
  transport::AdvertiseMsg noNodeUuidMsg(otherHeader, topic, addr, ctrl, "",
    scope, typeName, subscribers);
  buffer.resize(noNodeUuidMsg.GetMsgLength());
  EXPECT_EQ(0u, noNodeUuidMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvMsg (empty message type name).
  transport::AdvertiseMsg noTypeMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, "", subscribers);
  buffer.resize(noTypeMsg.GetMsgLength());
  EXPECT_EQ(0u, noTypeMsg.Pack(&buffer[0]));

  // Pack an AdvertiseMsg.
  transport::AdvertiseMsg advMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, typeName, subscribers);
  buffer.resize(advMsg.GetMsgLength());
  size_t bytes = advMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advMsg.GetMsgLength());

  // Unpack an AdvertiseMsg.
  transport::Header header;
  transport::AdvertiseMsg otherAdvMsg;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherAdvMsg.SetHeader(header);
  char *pBody = &buffer[0] + header.GetHeaderLength();
  size_t bodyBytes = otherAdvMsg.UnpackBody(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvMsg.GetTopic(), advMsg.GetTopic());
  EXPECT_EQ(otherAdvMsg.GetAddress(), advMsg.GetAddress());
  EXPECT_EQ(otherAdvMsg.GetControlAddress(), advMsg.GetControlAddress());
  EXPECT_EQ(otherAdvMsg.GetNodeUuid(), advMsg.GetNodeUuid());
  EXPECT_EQ(otherAdvMsg.GetScope(), advMsg.GetScope());
  EXPECT_EQ(otherAdvMsg.GetMsgTypeName(), advMsg.GetMsgTypeName());
  EXPECT_EQ(otherAdvMsg.GetMsgLength(), advMsg.GetMsgLength());
  EXPECT_EQ(otherAdvMsg.GetMsgLength() -
            otherAdvMsg.GetHeader().GetHeaderLength(), advMsg.GetMsgLength() -
            advMsg.GetHeader().GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvMsg.GetMsgLength() -
            otherAdvMsg.GetHeader().GetHeaderLength());

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
  transport::Header header = advSrv.GetHeader();
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

  EXPECT_EQ(advSrv.GetTopic(), topic);
  EXPECT_EQ(advSrv.GetAddress(), addr);
  EXPECT_EQ(advSrv.GetControlAddress(), ctrl);
  EXPECT_EQ(advSrv.GetNodeUuid(), nodeUuid);
  EXPECT_EQ(advSrv.GetScope(), scope);
  EXPECT_EQ(advSrv.GetReqTypeName(), reqType);
  EXPECT_EQ(advSrv.GetRepTypeName(), repType);

  size_t msgLength = advSrv.GetHeader().GetHeaderLength() +
    sizeof(uint64_t) + topic.size() +
    sizeof(uint64_t) + addr.size() +
    sizeof(uint64_t) + ctrl.size() +
    sizeof(uint64_t) + nodeUuid.size() +
    sizeof(uint8_t) +
    sizeof(uint64_t) + advSrv.GetReqTypeName().size() +
    sizeof(uint64_t) + advSrv.GetRepTypeName().size();
  EXPECT_EQ(advSrv.GetMsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvertiseSrv setters.
  transport::Header anotherHeader(version + 1, pUuid, transport::AdvSrvType, 3);
  advSrv.SetHeader(anotherHeader);
  header = advSrv.GetHeader();
  EXPECT_EQ(header.GetVersion(), version + 1);
  EXPECT_EQ(header.GetPUuid(), anotherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), transport::AdvSrvType);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint64_t) + header.GetPUuid().size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  topic = "a_new_topic_test";
  addr = "inproc://local";
  ctrl = "inproc://control";
  nodeUuid = "nodeUUID2";
  scope = transport::Scope::Host;
  reqType = "Type1";
  repType = "Type2";
  advSrv.SetTopic(topic);
  EXPECT_EQ(advSrv.GetTopic(), topic);
  advSrv.SetAddress(addr);
  EXPECT_EQ(advSrv.GetAddress(), addr);
  advSrv.SetControlAddress(ctrl);
  EXPECT_EQ(advSrv.GetControlAddress(), ctrl);
  advSrv.SetNodeUuid(nodeUuid);
  EXPECT_EQ(advSrv.GetNodeUuid(), nodeUuid);
  advSrv.SetScope(scope);
  EXPECT_EQ(advSrv.GetScope(), scope);
  advSrv.SetReqTypeName(reqType);
  EXPECT_EQ(advSrv.GetReqTypeName(), reqType);
  advSrv.SetRepTypeName(repType);
  EXPECT_EQ(advSrv.GetRepTypeName(), repType);

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
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0u);

  // Try to pack an incomplete AdvertiseSrv (empty request type).
  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);
  transport::AdvertiseSrv noReqMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, "", repType);
  buffer.resize(noReqMsg.GetMsgLength());
  EXPECT_EQ(0u, noReqMsg.Pack(&buffer[0]));

  // Try to pack an incomplete AdvertiseSrv (empty response type).
  transport::AdvertiseSrv noRepMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, reqType, "");
  buffer.resize(noRepMsg.GetMsgLength());
  EXPECT_EQ(0u, noRepMsg.Pack(&buffer[0]));

  // Pack an AdvertiseSrv.
  transport::AdvertiseSrv advSrv(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, reqType, repType);
  buffer.resize(advSrv.GetMsgLength());
  size_t bytes = advSrv.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advSrv.GetMsgLength());

  // Unpack an AdvertiseSrv.
  transport::Header header;
  transport::AdvertiseSrv otherAdvSrv;
  int headerBytes = header.Unpack(&buffer[0]);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherAdvSrv.SetHeader(header);
  char *pBody = &buffer[0] + header.GetHeaderLength();
  size_t bodyBytes = otherAdvSrv.UnpackBody(pBody);

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvSrv.GetTopic(), advSrv.GetTopic());
  EXPECT_EQ(otherAdvSrv.GetAddress(), advSrv.GetAddress());
  EXPECT_EQ(otherAdvSrv.GetControlAddress(), advSrv.GetControlAddress());
  EXPECT_EQ(otherAdvSrv.GetNodeUuid(), advSrv.GetNodeUuid());
  EXPECT_EQ(otherAdvSrv.GetScope(), advSrv.GetScope());
  EXPECT_EQ(otherAdvSrv.GetReqTypeName(), advSrv.GetReqTypeName());
  EXPECT_EQ(otherAdvSrv.GetRepTypeName(), advSrv.GetRepTypeName());
  EXPECT_EQ(otherAdvSrv.GetMsgLength(), advSrv.GetMsgLength());
  EXPECT_EQ(otherAdvSrv.GetMsgLength() -
            otherAdvSrv.GetHeader().GetHeaderLength(), advSrv.GetMsgLength() -
            advSrv.GetHeader().GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvSrv.GetMsgLength() -
            otherAdvSrv.GetHeader().GetHeaderLength());

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
