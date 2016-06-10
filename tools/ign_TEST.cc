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
#include <ignition/msgs.hh>

#include "gtest/gtest.h"
#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

std::string g_partition;
std::string g_topicCBStr;

/////////////////////////////////////////////////
std::string custom_exec_str(std::string _cmd)
{
  _cmd += " 2>&1";
  FILE *pipe = popen(_cmd.c_str(), "r");

  if (!pipe)
    return "ERROR";

  char buffer[128];
  std::string result = "";

  while (!feof(pipe))
  {
    if (fgets(buffer, 128, pipe) != NULL)
      result += buffer;
  }

  pclose(pipe);
  return result;
}

//////////////////////////////////////////////////
/// \brief Provide a service.
void srvEcho(const ignition::msgs::Int32 &_req, ignition::msgs::Int32 &_rep,
  bool &_result)
{
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Topic callback
void topicCB(const ignition::msgs::StringMsg &_msg)
{
  g_topicCBStr = _msg.data();
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -l' running the advertiser on a different process.
TEST(ignTest, TopicList)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'ign topic -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -l");
  EXPECT_EQ(output, "/foo\n");

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -i' running the advertiser on a different process.
TEST(ignTest, TopicInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'ign topic -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -t /foo -i");
  ASSERT_GT(output.size(), 50u);
  EXPECT_TRUE(output.find("ignition.msgs.Vector3d")
      != std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -l' running the advertiser on a different
/// process.
TEST(ignTest, ServiceList)
{
  // Launch a new responser process that advertises a service.
  std::string replier_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    g_partition.c_str());

  // Check the 'ign service -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -l");
  EXPECT_EQ(output, "/foo\n");

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -i' running the advertiser on a different process.
TEST(ignTest, ServiceInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string replier_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    g_partition.c_str());

  // Check the 'ign service -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -s /foo -i");
  ASSERT_GT(output.size(), 50u);
  EXPECT_TRUE(output.find("ignition.msgs.Int32") != std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -l' running the advertiser on the same process.
TEST(ignTest, TopicListSameProc)
{
  ignition::transport::Node node;

  ignition::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  EXPECT_TRUE(node.Advertise<ignition::msgs::Vector3d>("/foo"));
  EXPECT_TRUE(node.Publish("/foo", msg));

  // Check the 'ign topic -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -l");
  EXPECT_EQ(output, "/foo\n");
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -i' running the advertiser on the same process.
TEST(ignTest, TopicInfoSameProc)
{
  ignition::transport::Node node;

  ignition::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  EXPECT_TRUE(node.Advertise<ignition::msgs::Vector3d>("/foo"));
  EXPECT_TRUE(node.Publish("/foo", msg));

  // Check the 'ign topic -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -t /foo -i");

  ASSERT_GT(output.size(), 50u);
  EXPECT_TRUE(output.find("ignition.msgs.Vector3d") !=
      std::string::npos);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -l' running the advertiser on the same process.
TEST(ignTest, ServiceListSameProc)
{
  transport::Node node;
  EXPECT_TRUE(node.Advertise("/foo", srvEcho));

  // Check the 'ign service -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -l");
  EXPECT_EQ(output, "/foo\n");
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -i' running the advertiser on the same process.
TEST(ignTest, ServiceInfoSameProc)
{
  ignition::transport::Node node;
  EXPECT_TRUE(node.Advertise("/foo", srvEcho));

  // Check the 'ign service -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -s /foo -i");

  ASSERT_GT(output.size(), 50u);
  EXPECT_TRUE(output.find("ignition.msgs.Int32") != std::string::npos);
}


//////////////////////////////////////////////////
/// \brief Check 'ign topic -p' to send a message.
TEST(ignTest, TopicPublish)
{
  ignition::transport::Node node;
  g_topicCBStr = "bad_value";
  EXPECT_TRUE(node.Subscribe("/bar", topicCB));

  // Check the 'ign topic -p' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign +
      " topic -t /bar -m ign_msgs.StringMsg -p 'data:\"good_value\"'");

  ASSERT_TRUE(output.empty());
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  EXPECT_EQ(g_topicCBStr, "good_value");

  // Try to publish a message not included in Ignition Messages.
  std::string error = "Unable to create message of type";
  output = custom_exec_str(ign +
      " topic -t /bar -m ign_msgs.__bad_msg_type -p 'data:\"good_value\"'");
  EXPECT_TRUE(output.compare(0, error.size(), error) == 0);

  // Try to publish using an incorrect topic name.
  error = "Topic [/] is not valid";
  output = custom_exec_str(ign +
      " topic -t / wrong_topic -m ign_msgs.StringMsg -p 'data:\"good_value\"'");
  EXPECT_TRUE(output.compare(0, error.size(), error) == 0);
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
