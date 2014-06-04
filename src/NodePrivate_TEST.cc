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

#include <robot_msgs/stringmsg.pb.h>
#include <string>
#include <thread>
#include "gtest/gtest.h"
#include "ignition/transport/NodePrivate.hh"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Checking the helper functions for adding/deleting remote connections.
TEST(NodePrivateTest, Connections)
{
  std::string addr1 = "addr1";
  std::string addr2 = "addr2";
  std::string addr3 = "addr3";
  std::string addr4 = "addr4";
  std::string ctrl1 = "ctrl1";
  std::string ctrl2 = "ctrl2";
  std::string ctrl3 = "ctrl3";
  std::string ctrl4 = "ctrl4";
  std::string pUuid1 = "pUuid1";
  std::string pUuid2 = "pUuid2";
  std::string nUuid1 = "nUuid1";
  std::string nUuid2 = "nUuid2";
  std::string nUuid3 = "nUuid3";
  std::string nUuid4 = "nUuid4";
  transport::Scope scope = transport::Scope::All;

  transport::NodePrivate node;

  EXPECT_FALSE(node.Connected(addr1));
  node.AddConnection(pUuid1, addr1, ctrl1, nUuid1, scope);
  EXPECT_TRUE(node.Connected(addr1));
  node.AddConnection(pUuid1, addr2, ctrl2, nUuid2, scope);
  EXPECT_TRUE(node.Connected(addr2));
  node.AddConnection(pUuid2, addr3, ctrl3, nUuid3, scope);
  EXPECT_TRUE(node.Connected(addr3));
  node.AddConnection(pUuid2, addr4, ctrl4, nUuid4, scope);
  EXPECT_TRUE(node.Connected(addr4));

  node.DelConnection("", addr1);
  EXPECT_FALSE(node.Connected(addr1));
  EXPECT_TRUE(node.Connected(addr2));
  EXPECT_TRUE(node.Connected(addr3));
  EXPECT_TRUE(node.Connected(addr4));
  node.DelConnection("", addr2);
  EXPECT_FALSE(node.Connected(addr1));
  EXPECT_FALSE(node.Connected(addr2));
  EXPECT_TRUE(node.Connected(addr3));
  EXPECT_TRUE(node.Connected(addr4));
  node.DelConnection(pUuid2, "");
  EXPECT_FALSE(node.Connected(addr1));
  EXPECT_FALSE(node.Connected(addr2));
  EXPECT_FALSE(node.Connected(addr3));
  EXPECT_FALSE(node.Connected(addr4));
}

TEST(NodePrivateTest, Subnets)
{
  transport::NodePrivate node;

  EXPECT_TRUE(node.IsIPInRange("192.168.1.1", "192.168.1.0", "255.255.255.0"));
  EXPECT_FALSE(node.IsIPInRange("192.168.1.1", "192.168.1.2", "255.255.255.255"));
  EXPECT_FALSE(node.IsIPInRange("192.168.1.3", "192.168.1.2", "255.255.255.255"));
  EXPECT_FALSE(node.IsIPInRange("220.1.1.22", "192.168.1.0", "255.255.255.0"));
  EXPECT_TRUE(node.IsIPInRange("220.1.1.22", "220.1.1.22", "255.255.255.255"));
  EXPECT_FALSE(node.IsIPInRange("220.1.1.22", "220.1.1.23", "255.255.255.255"));
  EXPECT_FALSE(node.IsIPInRange("220.1.1.22", "220.1.1.21", "255.255.255.255"));
  EXPECT_TRUE(node.IsIPInRange("0.0.0.1", "0.0.0.0", "0.0.0.0"));
  EXPECT_FALSE(node.IsIPInRange("192.168.1.2", "10.0.0.1", "255.255.255.255"));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
