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
#include "gtest/gtest.h"

using namespace ignition;

std::string topic1 = "topic_test";
std::string addr1 = "tcp://10.0.0.1:6000";
std::string ctrl1 = "tcp://10.0.0.1:60011";
std::string pUuid1 = "procUUID";
std::string nUuid1 = "nodeUUID";
transport::Scope scope1 = transport::Scope::All;
std::string msgType1 = "StringMsg";
size_t msgHash1 = 1;
std::string reqType1 = "TypeReq";
size_t reqHash1 = 2;
std::string repType1 = "TypeRep";
size_t repHash1 = 3;

std::string topic2 = "topic_test_modified";
std::string addr2 = "tcp://10.0.0.2:6000";
std::string ctrl2 = "tcp://10.0.0.3:60011";
std::string pUuid2 = "procUUID-modified";
std::string nUuid2 = "nodeUUID-modified";
transport::Scope scope2 = transport::Scope::Host;
std::string msgType2 = "Int";
size_t msgHash2 = 4;
std::string reqType2 = "TypeReq-modified";
size_t reqHash2 = 5;
std::string repType2 = "TypeRep-modified";
size_t repHash2 = 6;

//////////////////////////////////////////////////
/// \brief Check the getters and setters.
TEST(TopicInfoTest, MsgTopicInfoBasicAPI)
{
  transport::MsgTopicInfo info(topic1, addr1, ctrl1, pUuid1, nUuid1,
    msgType1, msgHash1, scope1);

  // Check getters.
  EXPECT_EQ(topic1, info.GetTopic());
  EXPECT_EQ(addr1, info.GetAddr());
  EXPECT_EQ(ctrl1, info.GetCtrl());
  EXPECT_EQ(pUuid1, info.GetPUuid());
  EXPECT_EQ(nUuid1, info.GetNUuid());
  EXPECT_EQ(msgType1, info.GetMsgType());
  EXPECT_EQ(msgHash1, info.GetMsgHash());
  EXPECT_EQ(scope1, info.GetScope());

  // Check setters.
  info.SetTopic(topic2);
  info.SetAddr(addr2);
  info.SetCtrl(ctrl2);
  info.SetPUuid(pUuid2);
  info.SetNUuid(nUuid2);
  info.SetMsgType(msgType2);
  info.SetMsgHash(msgHash2);
  info.SetScope(scope2);
  EXPECT_EQ(topic2, info.GetTopic());
  EXPECT_EQ(addr2, info.GetAddr());
  EXPECT_EQ(ctrl2, info.GetCtrl());
  EXPECT_EQ(pUuid2, info.GetPUuid());
  EXPECT_EQ(nUuid2, info.GetNUuid());
  EXPECT_EQ(msgType2, info.GetMsgType());
  EXPECT_EQ(msgHash2, info.GetMsgHash());
  EXPECT_EQ(scope2, info.GetScope());
}

//////////////////////////////////////////////////
/// \brief Check the getters and setters.
TEST(TopicInfoTest, SrvTopicInfoBasicAPI)
{
  transport::SrvTopicInfo info(topic1, addr1, ctrl1, pUuid1, nUuid1,
    reqType1, reqHash1, repType1, repHash1, scope1);

  // Check getters.
  EXPECT_EQ(topic1, info.GetTopic());
  EXPECT_EQ(addr1, info.GetAddr());
  EXPECT_EQ(ctrl1, info.GetCtrl());
  EXPECT_EQ(pUuid1, info.GetPUuid());
  EXPECT_EQ(nUuid1, info.GetNUuid());
  EXPECT_EQ(reqType1, info.GetReqType());
  EXPECT_EQ(reqHash1, info.GetReqHash());
  EXPECT_EQ(repType1, info.GetRepType());
  EXPECT_EQ(repHash1, info.GetRepHash());
  EXPECT_EQ(scope1, info.GetScope());

  // Check setters.
  info.SetTopic(topic2);
  info.SetAddr(addr2);
  info.SetCtrl(ctrl2);
  info.SetPUuid(pUuid2);
  info.SetNUuid(nUuid2);
  info.SetReqType(reqType2);
  info.SetReqHash(reqHash2);
  info.SetRepType(repType2);
  info.SetRepHash(repHash2);
  info.SetScope(scope2);
  EXPECT_EQ(topic2, info.GetTopic());
  EXPECT_EQ(addr2, info.GetAddr());
  EXPECT_EQ(ctrl2, info.GetCtrl());
  EXPECT_EQ(pUuid2, info.GetPUuid());
  EXPECT_EQ(nUuid2, info.GetNUuid());
  EXPECT_EQ(reqType2, info.GetReqType());
  EXPECT_EQ(reqHash2, info.GetReqHash());
  EXPECT_EQ(repType2, info.GetRepType());
  EXPECT_EQ(repHash2, info.GetRepHash());
  EXPECT_EQ(scope2, info.GetScope());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
