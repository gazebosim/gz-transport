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
#include "gtest/gtest.h"
#include "ignition/transport/config.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"
#include "msg/int.pb.h"
#include "msg/vector3d.pb.h"

std::string partition;

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
void srvEcho(const std::string &_topic, const transport::msgs::Int &_req,
  transport::msgs::Int &_rep, bool &_result)
{
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic --info arg' running the advertiser on a separate
/// process
TEST(ignTest, TopicInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    partition.c_str());

  // Check the 'ign topic --info arg' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -i /foo");

  // Check that the output contains the type advertised.
  EXPECT_TRUE(
    output.find("Type: ignition.transport.msgs.Vector3d") != std::string::npos);

  // Check that there is only one publisher.
  EXPECT_EQ(output.find("Type: ignition.transport.msgs.Vector3d"),
            output.rfind("Type: ignition.transport.msgs.Vector3d"));

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic --list' running the advertiser on a separate process
TEST(ignTest, TopicList)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    partition.c_str());

  // Check the 'ign topic --list' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -l");
  EXPECT_EQ(output, "/foo\n");

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service --info arg' running the responser on a separate
/// process.
TEST(ignTest, ServiceInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string replier_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    partition.c_str());

  // Check the 'ign service --info arg' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -i /foo");

  // Check that the output contains the types advertised.
  EXPECT_TRUE(output.find("Request type: ignition.transport.msgs.Int") !=
    std::string::npos);
  EXPECT_TRUE(output.find("Response type: ignition.transport.msgs.Int") !=
    std::string::npos);

  // Check that there is only one responser.
  EXPECT_EQ(output.find("Request type: ignition.transport.msgs.Int"),
            output.rfind("Request type: ignition.transport.msgs.Int"));

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service --list' running the advertiser on a different
/// process.
TEST(ignTest, ServiceList)
{
  // Launch a new responser process that advertises a service.
  std::string replier_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    partition.c_str());

  // Check the 'ign service --list' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -l");
  EXPECT_EQ(output, "/foo\n");

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic --list' running the advertiser on the same process.
TEST(ignTest, TopicListSameProc)
{
  ignition::transport::Node node;

  ignition::transport::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  node.Advertise<ignition::transport::msgs::Vector3d>("/foo");
  node.Publish("/foo", msg);

  // Check the 'ign topic --list' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " topic -l");
  EXPECT_EQ(output, "/foo\n");
}

//////////////////////////////////////////////////
/// \brief Check 'ign service --list' running the advertiser on the same process
TEST(ignTest, ServiceListSameProc)
{
  transport::Node node;
  node.Advertise("/foo", srvEcho);

  // Check the 'ign service --list' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign + " service -l");
  EXPECT_EQ(output, "/foo\n");
}

/////////////////////////////////////////////////
/// Main
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomPartition();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  // Set IGN_CONFIG_PATH to the directory where the .yaml configuration files
  // is located.
  setenv("IGN_CONFIG_PATH", IGN_CONFIG_PATH, 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
