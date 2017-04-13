/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include <ignition/msgs.hh>

#include "gtest/gtest.h"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

// Global constants.
static const std::string  g_topic = "/topic";
static const std::string  service = "/service";
//static const char* _topic = g_topic.c_str();

// Global variables.
static std::string g_partition;

//////////////////////////////////////////////////
/// \brief Check cmdTopicInfo running the advertiser on a the same process.
TEST(ignTest, cmdTopicInfo)
{
  transport::Node node;
  //auto pub = node.Advertise<ignition::msgs::Int32>(g_topic);
  //EXPECT_TRUE(pub);

  // Redirect stdout.
  std::stringstream buffer;
  auto old = std::cout.rdbuf(buffer.rdbuf());

  cmdTopicInfo(nullptr);
  cmdTopicInfo(g_topic.c_str());

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "No publishers on topic [/topic]\n");

  // Restore stdout.
  std::cout.rdbuf(old);
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceInfo running the advertiser on a the same process.
TEST(ignTest, cmdServiceInfo)
{
  transport::Node node;
  
  // Redirect stdout.
  std::stringstream buffer;
  auto old = std::cout.rdbuf(buffer.rdbuf());

  cmdServiceInfo(nullptr);
  cmdServiceInfo(service.c_str());

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "No service providers on service [/service]\n");

  // Restore stdout.
  std::cout.rdbuf(old);
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicPub running the advertiser on a the same process.
TEST(ignTest, cmdTopicPub)
{
  std::string topic = "/topic";
  std::string msg_type = "/msgType";
  std::string msg_data = "/msgData";

  transport::Node node;
  
  // Redirect stdout.
  std::stringstream buffer;
  auto old = std::cout.rdbuf(buffer.rdbuf());

  cmdTopicPub(nullptr,msg_type.c_str(),msg_data.c_str());
  cmdTopicPub(topic.c_str(),nullptr,msg_data.c_str());
  cmdTopicPub(topic.c_str(),msg_type.c_str(),nullptr);
  
  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "");

  // Restore stdout.
  std::cout.rdbuf(old);
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq running the advertiser on a the same process.
TEST(ignTest, cmdServiceReq)
{
  std::string req_type = "/reqType";
  std::string rep_type = "/repType";
  std::string req_data = "/reqData";
  const int timeout = 10;

  transport::Node node;
  
  // Redirect stdout.
  std::stringstream buffer;
  auto old = std::cout.rdbuf(buffer.rdbuf());

  cmdServiceReq(nullptr,req_type.c_str(),rep_type.c_str(),timeout,req_data.c_str());
  cmdServiceReq(service.c_str(),nullptr,rep_type.c_str(),timeout,req_data.c_str());
  cmdServiceReq(service.c_str(),req_type.c_str(),nullptr,timeout,req_data.c_str());
  cmdServiceReq(service.c_str(),req_type.c_str(),rep_type.c_str(),timeout,nullptr);
  cmdServiceReq(service.c_str(),req_type.c_str(),rep_type.c_str(),timeout,req_data.c_str());
  //ToDo: cover few more lines

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "");

  // Restore stdout.
  std::cout.rdbuf(old);
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicEcho running the advertiser on a the same process.
TEST(ignTest, cmdTopicEcho)
{
  std::string invalid_topic ="/"; 
  transport::Node node;
  //auto pub = node.Advertise<ignition::msgs::Int32>(invalid_topic);
  //EXPECT_FALSE(pub);

  // Redirect stdout.
  std::stringstream buffer;
  auto old = std::cout.rdbuf(buffer.rdbuf());

  cmdTopicEcho(nullptr, 10.00);
  cmdTopicEcho(invalid_topic.c_str(), 5.00);
  
  // Restore stdout.
  std::cout.rdbuf(old);
}
/////////////////////////////////////////////////
/// Main
int main(int argc, char **argv)
{
  // Get a random partition name.
  g_partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", g_partition.c_str(), 1);

  // Set IGN_CONFIG_PATH to the directory where the .yaml configuration files
  // is located.
  setenv("IGN_CONFIG_PATH", IGN_CONFIG_PATH, 1);

  // Make sure that we load the library recently built and not the one installed
  // in your system.
#ifndef _WIN32
  setenv("LD_LIBRARY_PATH", IGN_TEST_LIBRARY_PATH, 1);
#endif

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
