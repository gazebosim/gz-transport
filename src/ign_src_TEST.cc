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
#include <string>
#include <iostream>
#include <sstream>
#include "gtest/gtest.h"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

// Global constants.
static const std::string  g_topic = "/topic";
static const std::string  service = "/echo";
static const std::string  service2 = "/timeout";

// Global variables.
static std::string g_partition;

/// \brief Provide a service.
void srvEcho(const ignition::msgs::Int32 &_req, ignition::msgs::Int32 &_rep,
  bool &_result)
{
  _rep.set_data(_req.data());
  _result = false;
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicInfo running the advertiser on a the same process.
TEST(ignTest, cmdTopicInfo)
{
  transport::Node node;

  // Redirect stdout.
  std::stringstream buffer;
  std::stringstream buffer2;
  auto old = std::cout.rdbuf(buffer.rdbuf());
  auto old2 = std::cerr.rdbuf(buffer2.rdbuf());

  cmdTopicInfo(nullptr);

  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Invalid topic. Topic must not be empty.\n");

  cmdTopicInfo(g_topic.c_str());

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "No publishers on topic [/topic]\n");

  // Restore stdout.
  std::cout.rdbuf(old);
  std::cerr.rdbuf(old2);
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceInfo running the advertiser on a the same process.
TEST(ignTest, cmdServiceInfo)
{
  transport::Node node;

  // Redirect stdout.
  std::stringstream buffer;
  std::stringstream buffer2;
  auto old = std::cout.rdbuf(buffer.rdbuf());
  auto old2 = std::cerr.rdbuf(buffer2.rdbuf());

  cmdServiceInfo(nullptr);

  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Invalid service. Service must not be empty.\n");

  cmdServiceInfo(service.c_str());

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "No service providers on service [/echo]\n");

  // Restore stdout.
  std::cout.rdbuf(old);
  std::cerr.rdbuf(old2);
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
  std::stringstream buffer2;

  auto old2 = std::cerr.rdbuf(buffer2.rdbuf());

  cmdTopicPub(nullptr, msg_type.c_str(), msg_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Topic name is null\n");

  buffer2.str(std::string());

  cmdTopicPub(topic.c_str(), nullptr, msg_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Message type is null\n");

  buffer2.str(std::string());

  cmdTopicPub(topic.c_str(), msg_type.c_str(), nullptr);
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Message data is null\n");

  // Restore stderr
  std::cerr.rdbuf(old2);
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq running the advertiser on a the same process.
TEST(ignTest, cmdServiceReq)
{
  std::string req_type = "/reqType";
  std::string rep_type = "/repType";
  std::string req_data = "/reqData";

  std::string rreq_type = "ign_msgs.Int32";
  std::string rrep_type = "ign_msgs.Int32";
  std::string rreq_data = "10";

  const int timeout = 10;

  transport::Node node;
  EXPECT_TRUE(node.Advertise(service, srvEcho));

  ignition::msgs::Int32 msg;
  msg.set_data(10);

  // Redirect stdout.
  std::stringstream buffer;
  std::stringstream buffer2;
  auto old = std::cout.rdbuf(buffer.rdbuf());
  auto old2 = std::cerr.rdbuf(buffer2.rdbuf());

  cmdServiceReq(nullptr, req_type.c_str(), rep_type.c_str(),
    timeout, req_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Service name is null\n");
  buffer2.str(std::string());

  cmdServiceReq(service.c_str(), nullptr, rep_type.c_str(),
    timeout, req_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Request type is null\n");
  buffer2.str(std::string());

  cmdServiceReq(service.c_str(), req_type.c_str(), nullptr,
    timeout, req_data.c_str());
  EXPECT_EQ(buffer2.str(), "Response type is null\n");
  buffer2.str(std::string());

  cmdServiceReq(service.c_str(), req_type.c_str(),
    rep_type.c_str(), timeout, nullptr);
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Request data is null\n");
  buffer2.str(std::string());

  cmdServiceReq(service.c_str(), req_type.c_str(),
    rep_type.c_str(), timeout, req_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(),
    "Unable to create request of type[/reqType] with data[/reqData].\n");
  buffer2.str(std::string());

  cmdServiceReq(service.c_str(), rreq_type.c_str(),
    rep_type.c_str(), timeout, rreq_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Unable to create response of type[/repType].\n");
  buffer2.str(std::string());

  cmdServiceReq(service.c_str(), rreq_type.c_str(),
    rrep_type.c_str(), timeout, rreq_data.c_str());

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "Service call failed\n");

  cmdServiceReq(service2.c_str(), rreq_type.c_str(),
    rrep_type.c_str(), timeout, rreq_data.c_str());
  // Verify that the stderr matches the expected output.
  EXPECT_EQ(buffer2.str(), "Service call timed out\n");

  // Restore stdout.
  std::cout.rdbuf(old);
  std::cerr.rdbuf(old2);
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicEcho running the advertiser on a the same process.
TEST(ignTest, cmdTopicEcho)
{
  std::string invalid_topic ="/";
  transport::Node node;

  // Redirect stdout.
  std::stringstream buffer;
  std::stringstream buffer2;
  auto old = std::cout.rdbuf(buffer.rdbuf());
  auto old2 = std::cerr.rdbuf(buffer2.rdbuf());

  cmdTopicEcho(nullptr, 10.00);
  EXPECT_EQ(buffer2.str(), "Invalid topic. Topic must not be empty.\n");

  cmdTopicEcho(invalid_topic.c_str(), 5.00);

  // Restore stdout.
  std::cout.rdbuf(old);
  std::cerr.rdbuf(old2);
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
