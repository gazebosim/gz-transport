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

#include <iostream>
#include <string>
#include "ignition/transport/TopicsInfo.hh"
#include "gtest/gtest.h"

using namespace ignition;

bool callbackExecuted = false;

//////////////////////////////////////////////////
void myReqCb(const std::string &/*_p1*/, int /*p2*/, const std::string &/*p3*/)
{
  callbackExecuted = true;
}

//////////////////////////////////////////////////
int myRepCb(const std::string &/*p1*/, const std::string &/*p2*/,
            std::string &/*p3*/)
{
  callbackExecuted = true;
  return 0;
}

//////////////////////////////////////////////////
/// \brief Check all the methods of the TopicsInfo helper class.
TEST(TopicsInfoTest, AddressInfoAPI)
{
  std::string topic  = "foo";

  std::string nUuid1 = "node-UUID-1";
  transport::Scope scope1 = transport::Scope::All;
  std::string nUuid2  = "node-UUID-2";
  transport::Scope scope2 = transport::Scope::Process;
  std::string nUuid3  = "node-UUID-3";
  transport::Scope scope3 = transport::Scope::Host;
  std::string nUuid4  = "node-UUID-4";
  transport::Scope scope4 = transport::Scope::All;

  std::string pUuid1 = "process-UUID-1";
  std::string addr1  = "tcp://10.0.0.1:6001";
  std::string ctrl1  = "tcp://10.0.0.1:60011";
  std::string pUuid2  = "process-UUID-2";
  std::string addr2  = "tcp://10.0.0.1:6002";
  std::string ctrl2  = "tcp://10.0.0.1:60022";

  transport::Addresses_M m;
  transport::Address_t info;
  transport::AddressInfo test;

  // Insert one node address.
  EXPECT_TRUE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid1, scope1));
  EXPECT_FALSE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid1, scope1));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);

  // Insert one node address on the same process.
  EXPECT_TRUE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid2, scope2));
  EXPECT_FALSE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid2, scope2));
  // Check GetAddress/
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.scope, scope2);

  // Insert a node address on a second process.
  EXPECT_TRUE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid3, scope3));
  EXPECT_FALSE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid3, scope3));
  // Check GetAddress.
  test.GetAddress(topic, pUuid2, nUuid3, info);
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid3);
  EXPECT_EQ(info.scope, scope3);

  // Insert another node on process2.
  EXPECT_TRUE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid4, scope4));
  EXPECT_FALSE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid4, scope4));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid2, nUuid4, info));
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid4);
  EXPECT_EQ(info.scope, scope4);

  // Remove the node4's address advertised for topic.
  test.DelAddressByNode(topic, pUuid2, nUuid4);
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_TRUE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid4, info));

  // Remove the node3's address advertised for topic.
  test.DelAddressByNode(topic, pUuid2, nUuid3);
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid4, info));

  // Remove all the addresses of process1.
  test.DelAddressesByProc(pUuid1);
  // Check GetAddress.
  EXPECT_FALSE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid4, info));
}

//////////////////////////////////////////////////
/// \brief Check all the methods of the TopicsInfo helper class.
TEST(TopicsInfoTest, BasicTopicsInfoAPI)
{
  transport::TopicsInfo topics;
  std::string topic  = "foo";

  transport::ReqCallback reqCb;
  transport::RepCallback repCb;

  // Check getters with an empty TopicsInfo object
  EXPECT_FALSE(topics.HasTopic(topic));
  EXPECT_FALSE(topics.Subscribed(topic));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
