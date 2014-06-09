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
#include "ignition/transport/TopicStorage.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check all the methods of the TopicStorage helper class.
TEST(TopicStorageTest, TopicStorageAPI)
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
  transport::TopicStorage test;

  // Check HasTopic.
  EXPECT_FALSE(test.HasTopic(topic));
  // Check HasAnyAddresses.
  EXPECT_FALSE(test.HasAnyAddresses(topic, pUuid1));
  // Check HasAddresses.
  EXPECT_FALSE(test.HasAddress(addr1));
  // Check GetAddress.
  EXPECT_FALSE(test.GetAddress(topic, pUuid1, nUuid1, info));
  // Check GetAddresses.
  EXPECT_FALSE(test.GetAddresses(topic, m));
  // Try to remove a entry not stored yet.
  test.DelAddressByNode(topic, pUuid2, nUuid4);
  // Try to remove a set of entries within a process not stored.
  test.DelAddressesByProc(pUuid1);

  // Insert one node address.
  EXPECT_TRUE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid1, scope1));
  EXPECT_FALSE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid1, scope1));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_TRUE(test.HasAddress(addr1));
  EXPECT_FALSE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);
  // Check GetAddresses.
  EXPECT_TRUE(test.GetAddresses(topic, m));
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(0).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(0).nUuid, nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).scope, scope1);

  // Insert one node address on the same process.
  EXPECT_TRUE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid2, scope2));
  EXPECT_FALSE(test.AddAddress(topic, addr1, ctrl1, pUuid1, nUuid2, scope2));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_TRUE(test.HasAddress(addr1));
  EXPECT_FALSE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.scope, scope2);
  // Check GetAddresses.
  EXPECT_TRUE(test.GetAddresses(topic, m));
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(0).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(0).nUuid, nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).scope, scope1);
  EXPECT_EQ(m[pUuid1].at(1).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(1).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(1).nUuid, nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).scope, scope2);

  // Insert a node address on a second process.
  EXPECT_TRUE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid3, scope3));
  EXPECT_FALSE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid3, scope3));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_TRUE(test.HasAddress(addr1));
  EXPECT_TRUE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.scope, scope2);
  EXPECT_TRUE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid3);
  EXPECT_EQ(info.scope, scope3);
  // Check GetAddresses.
  EXPECT_TRUE(test.GetAddresses(topic, m));
  EXPECT_EQ(m.size(), 2);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(0).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(0).nUuid, nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).scope, scope1);
  EXPECT_EQ(m[pUuid1].at(1).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(1).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(1).nUuid, nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).scope, scope2);
  EXPECT_EQ(m[pUuid2].at(0).addr, addr2);
  EXPECT_EQ(m[pUuid2].at(0).ctrl, ctrl2);
  EXPECT_EQ(m[pUuid2].at(0).nUuid, nUuid3);
  EXPECT_EQ(m[pUuid2].at(0).scope, scope3);

  // Insert another node on process2.
  EXPECT_TRUE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid4, scope4));
  EXPECT_FALSE(test.AddAddress(topic, addr2, ctrl2, pUuid2, nUuid4, scope4));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_TRUE(test.HasAddress(addr1));
  EXPECT_TRUE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.scope, scope2);
  EXPECT_TRUE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid3);
  EXPECT_EQ(info.scope, scope3);
  EXPECT_TRUE(test.GetAddress(topic, pUuid2, nUuid4, info));
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid4);
  EXPECT_EQ(info.scope, scope4);
  // Check GetAddresses.
  EXPECT_TRUE(test.GetAddresses(topic, m));
  EXPECT_EQ(m.size(), 2);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(0).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(0).nUuid, nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).scope, scope1);
  EXPECT_EQ(m[pUuid1].at(1).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(1).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(1).nUuid, nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).scope, scope2);
  EXPECT_EQ(m[pUuid2].at(0).addr, addr2);
  EXPECT_EQ(m[pUuid2].at(0).ctrl, ctrl2);
  EXPECT_EQ(m[pUuid2].at(0).nUuid, nUuid3);
  EXPECT_EQ(m[pUuid2].at(0).scope, scope3);
  EXPECT_EQ(m[pUuid2].at(1).addr, addr2);
  EXPECT_EQ(m[pUuid2].at(1).ctrl, ctrl2);
  EXPECT_EQ(m[pUuid2].at(1).nUuid, nUuid4);
  EXPECT_EQ(m[pUuid2].at(1).scope, scope4);

  // Check that Print() does not break anything.
  test.Print();

  // Remove the node4's address advertised for topic.
  test.DelAddressByNode(topic, pUuid2, nUuid4);
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_TRUE(test.HasAddress(addr1));
  EXPECT_TRUE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.scope, scope2);
  EXPECT_TRUE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid3);
  EXPECT_EQ(info.scope, scope3);
  // Check GetAddresses.
  EXPECT_TRUE(test.GetAddresses(topic, m));
  EXPECT_EQ(m.size(), 2);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(0).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(0).nUuid, nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).scope, scope1);
  EXPECT_EQ(m[pUuid1].at(1).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(1).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(1).nUuid, nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).scope, scope2);
  EXPECT_EQ(m[pUuid2].at(0).addr, addr2);
  EXPECT_EQ(m[pUuid2].at(0).ctrl, ctrl2);
  EXPECT_EQ(m[pUuid2].at(0).nUuid, nUuid3);
  EXPECT_EQ(m[pUuid2].at(0).scope, scope3);

  // Remove the node3's address advertised for topic.
  test.DelAddressByNode(topic, pUuid2, nUuid3);
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_TRUE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_TRUE(test.HasAddress(addr1));
  EXPECT_FALSE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.scope, scope1);
  EXPECT_TRUE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.scope, scope2);
  // Check GetAddresses.
  EXPECT_TRUE(test.GetAddresses(topic, m));
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(0).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(0).nUuid, nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).scope, scope1);
  EXPECT_EQ(m[pUuid1].at(1).addr, addr1);
  EXPECT_EQ(m[pUuid1].at(1).ctrl, ctrl1);
  EXPECT_EQ(m[pUuid1].at(1).nUuid, nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).scope, scope2);

  // Remove all the addresses of process1.
  test.DelAddressesByProc(pUuid1);
  // Check HasTopic.
  EXPECT_FALSE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyAddresses.
  EXPECT_FALSE(test.HasAnyAddresses(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyAddresses(topic, pUuid2));
  // Check HasAddresses.
  EXPECT_FALSE(test.HasAddress(addr1));
  EXPECT_FALSE(test.HasAddress(addr2));
  // Check GetAddress.
  EXPECT_FALSE(test.GetAddress(topic, pUuid1, nUuid1, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid1, nUuid2, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid3, info));
  EXPECT_FALSE(test.GetAddress(topic, pUuid2, nUuid4, info));
  // Check GetAddresses.
  EXPECT_FALSE(test.GetAddresses(topic, m));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
