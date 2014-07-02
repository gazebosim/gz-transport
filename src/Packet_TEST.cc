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
#include "ignition/transport/Packet.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the getters and setters.
TEST(PacketTest, BasicHeaderAPI)
{
  std::string topic = "topic_test";
  std::string pUuid = "Process-UUID-1";
  transport::Header header(transport::Version, pUuid,
    topic, transport::AdvType);

  // Check Header getters.
  EXPECT_EQ(header.GetVersion(), transport::Version);
  EXPECT_EQ(pUuid, header.GetPUuid());
  EXPECT_EQ(header.GetTopicLength(), topic.size());
  EXPECT_EQ(header.GetTopic(), topic);
  EXPECT_EQ(header.GetType(), transport::AdvType);
  EXPECT_EQ(header.GetFlags(), 0);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint16_t) + header.GetPUuid().size() +
    sizeof(header.GetTopicLength()) + topic.size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  // Check Header setters.
  header.SetVersion(transport::Version + 1);
  EXPECT_EQ(header.GetVersion(), transport::Version + 1);
  pUuid = "Different-process-UUID-1";
  header.SetPUuid(pUuid);
  EXPECT_EQ(header.GetPUuid(), pUuid);
  topic = "a_new_topic_test";
  header.SetTopic(topic);
  EXPECT_EQ(header.GetTopic(), topic);
  EXPECT_EQ(header.GetTopicLength(), topic.size());
  header.SetType(transport::SubType);
  EXPECT_EQ(header.GetType(), transport::SubType);
  header.SetFlags(1);
  EXPECT_EQ(header.GetFlags(), 1);
  headerLength = sizeof(header.GetVersion()) +
    sizeof(uint16_t) + header.GetPUuid().size() +
    sizeof(header.GetTopicLength()) + topic.size() +
    sizeof(header.GetType()) + sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  // Check << operator
  std::ostringstream output;
  output << header;
  std::string expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tTopic length: 16\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tType: SUBSCRIBE\n"
    "\tFlags: 1\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of a header.
TEST(PacketTest, HeaderIO)
{
  std::string topic = "topic_test";
  std::string pUuid = "Process-UUID-1";
  char *buffer = nullptr;

  // Try to pack an empty header.
  transport::Header emptyHeader;
  EXPECT_EQ(emptyHeader.Pack(buffer), 0);

  // Pack a Header
  transport::Header header(transport::Version, pUuid, topic,
    transport::AdvSrvType, 2);
  buffer = new char[header.GetHeaderLength()];
  size_t bytes = header.Pack(buffer);
  EXPECT_EQ(bytes, header.GetHeaderLength());

  // Unpack the Header
  transport::Header otherHeader;
  otherHeader.Unpack(buffer);
  delete[] buffer;

  // Check that after Pack() and Unpack() the Header remains the same
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetTopicLength(), otherHeader.GetTopicLength());
  EXPECT_EQ(header.GetTopic(), otherHeader.GetTopic());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV message.
TEST(PacketTest, BasicAdvMsgAPI)
{
  std::string topic = "topic_test";
  std::string pUuid = "Process-UUID-1";

  transport::Header otherHeader(transport::Version, pUuid, topic,
    transport::AdvType, 3);

  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::All;
  transport::AdvMsg advMsg(otherHeader, addr, ctrl, nodeUuid, scope);

  // Check AdvMsg getters.
  transport::Header header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), otherHeader.GetVersion());
  EXPECT_EQ(header.GetPUuid(), otherHeader.GetPUuid());
  EXPECT_EQ(header.GetTopicLength(), otherHeader.GetTopicLength());
  EXPECT_EQ(header.GetTopic(), otherHeader.GetTopic());
  EXPECT_EQ(header.GetType(), otherHeader.GetType());
  EXPECT_EQ(header.GetFlags(), otherHeader.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), otherHeader.GetHeaderLength());

  EXPECT_EQ(advMsg.GetAddressLength(), addr.size());
  EXPECT_EQ(advMsg.GetAddress(), addr);
  EXPECT_EQ(advMsg.GetControlAddressLength(), ctrl.size());
  EXPECT_EQ(advMsg.GetControlAddress(), ctrl);
  EXPECT_EQ(advMsg.GetNodeUuidLength(), nodeUuid.size());
  EXPECT_EQ(advMsg.GetNodeUuid(), nodeUuid);
  EXPECT_EQ(advMsg.GetScope(), scope);

  size_t msgLength = advMsg.GetHeader().GetHeaderLength() +
    sizeof(advMsg.GetAddressLength()) + addr.size() +
    sizeof(advMsg.GetControlAddressLength()) + ctrl.size() +
    sizeof(advMsg.GetNodeUuidLength()) + nodeUuid.size() +
    sizeof(advMsg.GetScope());
  EXPECT_EQ(advMsg.GetMsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";
  topic = "a_new_topic_test";

  // Check AdvMsg setters.
  transport::Header anotherHeader(transport::Version + 1, pUuid,
    topic, transport::AdvSrvType, 3);
  advMsg.SetHeader(anotherHeader);
  header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), transport::Version+ 1);
  EXPECT_EQ(header.GetPUuid(), anotherHeader.GetPUuid());
  EXPECT_EQ(header.GetTopicLength(), topic.size());
  EXPECT_EQ(header.GetTopic(), topic);
  EXPECT_EQ(header.GetType(), transport::AdvSrvType);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(uint16_t) + header.GetPUuid().size() +
    sizeof(header.GetTopicLength()) + topic.size() + sizeof(header.GetType()) +
    sizeof(header.GetFlags());
  EXPECT_EQ(header.GetHeaderLength(), headerLength);

  addr = "inproc://local";
  ctrl = "inproc://control";
  nodeUuid = "nodeUUID2";
  scope = transport::Scope::Host;
  advMsg.SetAddress(addr);
  EXPECT_EQ(advMsg.GetAddress(), addr);
  advMsg.SetControlAddress(ctrl);
  EXPECT_EQ(advMsg.GetControlAddress(), ctrl);
  advMsg.SetNodeUuid(nodeUuid);
  EXPECT_EQ(advMsg.GetNodeUuid(), nodeUuid);
  advMsg.SetScope(scope);
  EXPECT_EQ(advMsg.GetScope(), scope);

  // Check << operator
  std::ostringstream output;
  output << advMsg;
  std::cout << advMsg;
  std::string expectedOutput =
    "--------------------------------------\n"
    "Header:\n"
    "\tVersion: 2\n"
    "\tProcess UUID: Different-process-UUID-1\n"
    "\tTopic length: 16\n"
    "\tTopic: [a_new_topic_test]\n"
    "\tType: ADV_SVC\n"
    "\tFlags: 3\n"
    "Body:\n"
    "\tAddr size: 14\n"
    "\tAddress: inproc://local\n"
    "\tControl addr size: 16\n"
    "\tControl address: inproc://control\n"
    "\tNode UUID: nodeUUID2\n"
    "\tTopic Scope: Host\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of an ADV message.
TEST(PacketTest, AdvMsgIO)
{
  std::string pUuid = "Process-UUID-1";
  std::string topic = "topic_test";
  char *buffer = nullptr;

  // Try to pack an empty AdvMsg.
  transport::AdvMsg emptyMsg;
  EXPECT_EQ(emptyMsg.Pack(buffer), 0);

  // Pack an AdvMsg.
  transport::Header otherHeader(transport::Version, pUuid, topic,
    transport::AdvType, 3);
  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::Host;

  transport::AdvMsg advMsg(otherHeader, addr, ctrl, nodeUuid, scope);
  buffer = new char[advMsg.GetMsgLength()];
  size_t bytes = advMsg.Pack(buffer);
  EXPECT_EQ(bytes, advMsg.GetMsgLength());

  // Unpack an AdvMsg.
  transport::Header header;
  transport::AdvMsg otherAdvMsg;
  size_t headerBytes = header.Unpack(buffer);
  EXPECT_EQ(headerBytes, header.GetHeaderLength());
  otherAdvMsg.SetHeader(header);
  char *pBody = buffer + header.GetHeaderLength();
  size_t bodyBytes = otherAdvMsg.UnpackBody(pBody);
  delete[] buffer;

  // Check that after Pack() and Unpack() the data does not change.
  EXPECT_EQ(otherAdvMsg.GetAddressLength(), advMsg.GetAddressLength());
  EXPECT_EQ(otherAdvMsg.GetAddress(), advMsg.GetAddress());
  EXPECT_EQ(otherAdvMsg.GetControlAddressLength(),
            advMsg.GetControlAddressLength());
  EXPECT_EQ(otherAdvMsg.GetControlAddress(), advMsg.GetControlAddress());
  EXPECT_EQ(otherAdvMsg.GetNodeUuidLength(), advMsg.GetNodeUuidLength());
  EXPECT_EQ(otherAdvMsg.GetNodeUuid(), advMsg.GetNodeUuid());
  EXPECT_EQ(otherAdvMsg.GetScope(), advMsg.GetScope());
  EXPECT_EQ(otherAdvMsg.GetMsgLength(), advMsg.GetMsgLength());
  EXPECT_EQ(otherAdvMsg.GetMsgLength() -
            otherAdvMsg.GetHeader().GetHeaderLength(), advMsg.GetMsgLength() -
            advMsg.GetHeader().GetHeaderLength());
  EXPECT_EQ(bodyBytes, otherAdvMsg.GetMsgLength() -
            otherAdvMsg.GetHeader().GetHeaderLength());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
