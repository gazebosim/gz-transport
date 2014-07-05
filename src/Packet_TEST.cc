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
    sizeof(size_t) + header.GetPUuid().size() +
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
    sizeof(size_t) + header.GetPUuid().size() +
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
  EXPECT_EQ(emptyHeader.Pack(&buffer[0]), 0);

  // Pack a Header.
  transport::Header header(version, pUuid, transport::AdvSrvType, 2);

  buffer.resize(header.GetHeaderLength());
  size_t bytes = header.Pack(&buffer[0]);
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
}

//////////////////////////////////////////////////
/// \brief Check the basic API for creating/reading an ADV message.
TEST(PacketTest, BasicAdvMsgAPI)
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
  transport::AdvMsg advMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, typeName);

  // Check AdvMsg getters.
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
    sizeof(size_t) + topic.size() +
    sizeof(size_t) + addr.size() +
    sizeof(size_t) + ctrl.size() +
    sizeof(size_t) + nodeUuid.size() +
    sizeof(advMsg.GetScope()) +
    sizeof(size_t) + advMsg.GetMsgTypeName().size();
  EXPECT_EQ(advMsg.GetMsgLength(), msgLength);

  pUuid = "Different-process-UUID-1";

  // Check AdvMsg setters.
  transport::Header anotherHeader(version + 1, pUuid, transport::AdvSrvType, 3);
  advMsg.SetHeader(anotherHeader);
  header = advMsg.GetHeader();
  EXPECT_EQ(header.GetVersion(), version + 1);
  EXPECT_EQ(header.GetPUuid(), anotherHeader.GetPUuid());
  EXPECT_EQ(header.GetType(), transport::AdvSrvType);
  EXPECT_EQ(header.GetFlags(), 3);
  int headerLength = sizeof(header.GetVersion()) +
    sizeof(size_t) + header.GetPUuid().size() +
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
  std::cout << advMsg;
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
}

//////////////////////////////////////////////////
/// \brief Check the serialization and unserialization of an ADV message.
TEST(PacketTest, AdvMsgIO)
{
  std::string pUuid = "Process-UUID-1";
  uint8_t version   = 1;

  // Try to pack an empty AdvMsg.
  transport::AdvMsg emptyMsg;
  std::vector<char> buffer(emptyMsg.GetMsgLength());
  EXPECT_EQ(emptyMsg.Pack(&buffer[0]), 0);

  // Pack an AdvMsg.
  transport::Header otherHeader(version, pUuid, transport::AdvType, 3);
  std::string topic = "topic_test";
  std::string addr = "tcp://10.0.0.1:6000";
  std::string ctrl = "tcp://10.0.0.1:60011";
  std::string nodeUuid = "nodeUUID";
  transport::Scope scope = transport::Scope::Host;
  std::string typeName = "StringMsg";

  transport::AdvMsg advMsg(otherHeader, topic, addr, ctrl, nodeUuid,
    scope, typeName);
  buffer.resize(advMsg.GetMsgLength());
  size_t bytes = advMsg.Pack(&buffer[0]);
  EXPECT_EQ(bytes, advMsg.GetMsgLength());

  // Unpack an AdvMsg.
  transport::Header header;
  transport::AdvMsg otherAdvMsg;
  size_t headerBytes = header.Unpack(&buffer[0]);
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
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
