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
#include "gtest/gtest.h"
#include "ignition/transport/NetUtils.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
/// \brief Checking the functions for dealing with IPs and subnets.
/*TEST(NodePrivateTest, Subnets)
{
  EXPECT_TRUE(
    NetUtils::IsIpInRange("192.168.1.1", "192.168.1.0", "255.255.255.0"));
  EXPECT_FALSE(
    NetUtils::IsIpInRange("192.168.1.1", "192.168.1.2", "255.255.255.255"));
  EXPECT_FALSE(
    NetUtils::IsIpInRange("192.168.1.3", "192.168.1.2", "255.255.255.255"));
  EXPECT_FALSE(
    NetUtils::IsIpInRange("220.1.1.22", "192.168.1.0", "255.255.255.0"));
  EXPECT_TRUE(
    NetUtils::IsIpInRange("220.1.1.22", "220.1.1.22", "255.255.255.255"));
  EXPECT_FALSE(
    NetUtils::IsIpInRange("220.1.1.22", "220.1.1.23", "255.255.255.255"));
  EXPECT_FALSE(
    NetUtils::IsIpInRange("220.1.1.22", "220.1.1.21", "255.255.255.255"));
  EXPECT_TRUE(
    NetUtils::IsIpInRange("0.0.0.1", "0.0.0.0", "0.0.0.0"));
  EXPECT_FALSE(
    NetUtils::IsIpInRange("192.168.1.2", "10.0.0.1", "255.255.255.255"));
}*/

//////////////////////////////////////////////////
/// \brief Checking the 0MQ utilites.
TEST(NodePrivateTest, zmqUtils)
{
  std::string zmqEndPoint1 = "tcp://192.168.1.1:29222";
  std::string ip;
  EXPECT_TRUE(NetUtils::ZmqToIp(zmqEndPoint1, ip));
  EXPECT_EQ(ip, "192.168.1.1");
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
