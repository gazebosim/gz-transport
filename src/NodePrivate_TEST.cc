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

TEST(DiszZmqTest, Connections)
{
  std::string addr1 = "addr1";
  std::string addr2 = "addr2";
  std::string addr3 = "addr3";
  std::string addr4 = "addr4";
  std::string ctrl1 = "ctrl1";
  std::string ctrl2 = "ctrl2";
  std::string ctrl3 = "ctrl3";
  std::string ctrl4 = "ctrl4";
  std::string uuid1 = "uuid1";
  std::string uuid2 = "uuid2";
  std::string uuid3 = "uuid3";
  std::string uuid4 = "uuid4";

  transport::NodePrivate node;

  EXPECT_FALSE(node.Connected(addr1));
  node.AddConnection(uuid1, addr1, ctrl1);
  EXPECT_TRUE(node.Connected(addr1));
  node.AddConnection(uuid1, addr2, ctrl2);
  EXPECT_TRUE(node.Connected(addr2));
  node.AddConnection(uuid2, addr3, ctrl3);
  EXPECT_TRUE(node.Connected(addr3));
  node.AddConnection(uuid2, addr4, ctrl4);
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
  node.DelConnection(uuid2, "");
  EXPECT_FALSE(node.Connected(addr1));
  EXPECT_FALSE(node.Connected(addr2));
  EXPECT_FALSE(node.Connected(addr3));
  EXPECT_FALSE(node.Connected(addr4));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
