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

#include <fstream>
#include <iostream>
#include <string>
#include <ignition/msgs.hh>
#include <ignition/utilities/ExtraTestMacros.hh>

#include "gtest/gtest.h"
#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

#ifdef _MSC_VER
#    define popen _popen
#    define pclose _pclose
#endif

using namespace ignition;

static std::string g_partition; // NOLINT(*)
static std::string g_topicCBStr; // NOLINT(*)
static const std::string g_ignVersion("--force-version " + // NOLINT(*)
  std::string(IGN_VERSION_FULL));

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
bool srvEcho(const ignition::msgs::Int32 &_req, ignition::msgs::Int32 &_rep)
{
  _rep.set_data(_req.data());
  return true;
}

//////////////////////////////////////////////////
/// \brief Topic callback
void topicCB(const ignition::msgs::StringMsg &_msg)
{
  g_topicCBStr = _msg.data();
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -l' running the advertiser on a different process.
TEST(ignTest, IGN_UTILS_TEST_DISABLED_ON_MAC(TopicList))
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    IGN_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'ign topic -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool topicFound = false;

  while (!topicFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(ign + " topic -l " + g_ignVersion);
    topicFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(topicFound);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -i' running the advertiser on a different process.
TEST(ignTest, TopicInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    IGN_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'ign topic -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(ign + " topic -t /foo -i " + g_ignVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound) << "OUTPUT["
    << output << "] Size[" << output.size()
    << "]. Expected Size=50" << std::endl;
  EXPECT_TRUE(output.find("ignition.msgs.Vector3d") != std::string::npos);

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
    IGN_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    g_partition.c_str());

  // Check the 'ign service -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool serviceFound = false;

  while (!serviceFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(ign + " service -l " + g_ignVersion);
    serviceFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(serviceFound);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -i' running the advertiser on a different process.
TEST(ignTest, ServiceInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string replier_path = testing::portablePathUnion(
    IGN_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    g_partition.c_str());

  // Check the 'ign service -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(ign + " service -s /foo -i " + g_ignVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound);
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

  auto pub = node.Advertise<ignition::msgs::Vector3d>("/foo");
  EXPECT_TRUE(pub);
  EXPECT_TRUE(pub.Publish(msg));

  // Check the 'ign topic -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool topicFound = false;

  while (!topicFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(ign + " topic -l " + g_ignVersion);
    topicFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(topicFound);
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

  auto pub = node.Advertise<ignition::msgs::Vector3d>("/foo");
  EXPECT_TRUE(pub);
  EXPECT_TRUE(pub.Publish(msg));

  // Check the 'ign topic -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(ign + " topic -t /foo -i " + g_ignVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound);
  EXPECT_TRUE(output.find("ignition.msgs.Vector3d") != std::string::npos);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -l' running the advertiser on the same process.
TEST(ignTest, ServiceListSameProc)
{
  transport::Node node;
  EXPECT_TRUE(node.Advertise("/foo", srvEcho));

  // Check the 'ign service -l' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool serviceFound = false;

  while (!serviceFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(ign + " service -l " + g_ignVersion);
    serviceFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(serviceFound);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -i' running the advertiser on the same process.
TEST(ignTest, ServiceInfoSameProc)
{
  ignition::transport::Node node;
  EXPECT_TRUE(node.Advertise("/foo", srvEcho));

  // Check the 'ign service -i' command.
  std::string ign = std::string(IGN_PATH) + "/ign";

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(ign + " service -s /foo -i " + g_ignVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound);
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
      " topic -t /bar -m ign_msgs.StringMsg -p 'data:\"good_value\"' " +
      g_ignVersion);

  ASSERT_TRUE(output.empty()) << output;
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  EXPECT_EQ(g_topicCBStr, "good_value");

  // Try to publish a message not included in Ignition Messages.
  std::string error = "Unable to create message of type";
  output = custom_exec_str(ign +
      " topic -t /bar -m ign_msgs.__bad_msg_type -p 'data:\"good_value\"' " +
      g_ignVersion);
  EXPECT_EQ(output.compare(0, error.size(), error), 0);

  // Try to publish using an incorrect topic name.
  error = "Topic [/] is not valid";
  output = custom_exec_str(ign +
      " topic -t / wrong_topic -m ign_msgs.StringMsg -p 'data:\"good_value\"' "+
      g_ignVersion);
  EXPECT_EQ(output.compare(0, error.size(), error), 0);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service -r' to request a service.
TEST(ignTest, ServiceRequest)
{
  ignition::transport::Node node;

  // Advertise a service.
  std::string service = "/echo";
  std::string value = "10";
  EXPECT_TRUE(node.Advertise(service, srvEcho));

  ignition::msgs::Int32 msg;
  msg.set_data(10);

  // Check the 'ign service -r' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(ign +
      " service -s " + service + " --reqtype ign_msgs.Int32 " +
      "--reptype ign_msgs.Int32 --timeout 1000 " +
      "--req 'data: " + value + "' " + g_ignVersion);

  ASSERT_EQ(output, "data: " + value + "\n\n");
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -e' running the publisher on a separate process.
TEST(ignTest, TopicEcho)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    IGN_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'ign topic -e' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(
    ign + " topic -e -t /foo -d 1.5 " + g_ignVersion);

  EXPECT_TRUE(output.find("x: 1") != std::string::npos);
  EXPECT_TRUE(output.find("y: 2") != std::string::npos);
  EXPECT_TRUE(output.find("z: 3") != std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic -e -n 2' running the publisher on a separate
/// process.
TEST(ignTest, TopicEchoNum)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    IGN_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'ign topic -e -n' command.
  std::string ign = std::string(IGN_PATH) + "/ign";
  std::string output = custom_exec_str(
    ign + " topic -e -t /foo -n 2 " + g_ignVersion);

  size_t pos = output.find("x: 1");
  EXPECT_TRUE(pos != std::string::npos);
  pos = output.find("x: 1", pos + 4);
  EXPECT_TRUE(pos != std::string::npos);
  pos = output.find("x: 1", pos + 4);
  EXPECT_TRUE(pos == std::string::npos);

  pos = output.find("y: 2");
  EXPECT_TRUE(pos != std::string::npos);
  pos = output.find("y: 2", pos + 4);
  EXPECT_TRUE(pos != std::string::npos);
  pos = output.find("y: 2", pos + 4);
  EXPECT_TRUE(pos == std::string::npos);

  pos = output.find("z: 3");
  EXPECT_TRUE(pos != std::string::npos);
  pos = output.find("z: 3", pos + 4);
  EXPECT_TRUE(pos != std::string::npos);
  pos = output.find("z: 3", pos + 4);
  EXPECT_TRUE(pos == std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'ign service --help' message and bash completion script for
/// consistent flags
TEST(ignTest, ServiceHelpVsCompletionFlags)
{
  // Flags in help message
  std::string output = custom_exec_str("ign service --help" + g_ignVersion);
  EXPECT_NE(std::string::npos, output.find("--help")) << output;
  EXPECT_NE(std::string::npos, output.find("--version")) << output;
  EXPECT_NE(std::string::npos, output.find("--service")) << output;
  EXPECT_NE(std::string::npos, output.find("--reqtype")) << output;
  EXPECT_NE(std::string::npos, output.find("--reptype")) << output;
  EXPECT_NE(std::string::npos, output.find("--timeout")) << output;
  EXPECT_NE(std::string::npos, output.find("--list")) << output;
  EXPECT_NE(std::string::npos, output.find("--info")) << output;
  EXPECT_NE(std::string::npos, output.find("--req")) << output;
  // TODO(anyone): In Fortress+, remove --force-version and --versions.
  // Add --help-all. Update cmd/transport.bash_completion.sh accordingly.
  EXPECT_NE(std::string::npos, output.find("--force-version")) << output;
  EXPECT_NE(std::string::npos, output.find("--versions")) << output;

  // Flags in bash completion
  std::ifstream scriptFile(std::string(IGN_TRANSPORT_SOURCE_DIR) +
    "/src/cmd/transport.bash_completion.sh");
  std::string script((std::istreambuf_iterator<char>(scriptFile)),
      std::istreambuf_iterator<char>());

  EXPECT_NE(std::string::npos, script.find("--help")) << script;
  EXPECT_NE(std::string::npos, script.find("--version")) << script;
  EXPECT_NE(std::string::npos, script.find("--service")) << script;
  EXPECT_NE(std::string::npos, script.find("--reqtype")) << script;
  EXPECT_NE(std::string::npos, script.find("--reptype")) << script;
  EXPECT_NE(std::string::npos, script.find("--timeout")) << script;
  EXPECT_NE(std::string::npos, script.find("--list")) << script;
  EXPECT_NE(std::string::npos, script.find("--info")) << script;
  EXPECT_NE(std::string::npos, script.find("--req")) << script;
  // TODO(anyone): In Fortress+, remove --force-version and --versions.
  // Add --help-all. Update cmd/transport.bash_completion.sh accordingly.
  EXPECT_NE(std::string::npos, script.find("--force-version")) << script;
  EXPECT_NE(std::string::npos, script.find("--versions")) << script;
}

//////////////////////////////////////////////////
/// \brief Check 'ign topic --help' message and bash completion script for
/// consistent flags
TEST(ignTest, TopicHelpVsCompletionFlags)
{
  // Flags in help message
  std::string output = custom_exec_str("ign topic --help" + g_ignVersion);
  EXPECT_NE(std::string::npos, output.find("--help")) << output;
  EXPECT_NE(std::string::npos, output.find("--version")) << output;
  EXPECT_NE(std::string::npos, output.find("--topic")) << output;
  EXPECT_NE(std::string::npos, output.find("--msgtype")) << output;
  EXPECT_NE(std::string::npos, output.find("--duration")) << output;
  EXPECT_NE(std::string::npos, output.find("--num")) << output;
  EXPECT_NE(std::string::npos, output.find("--list")) << output;
  EXPECT_NE(std::string::npos, output.find("--info")) << output;
  EXPECT_NE(std::string::npos, output.find("--echo")) << output;
  EXPECT_NE(std::string::npos, output.find("--pub")) << output;
  // NOTE: In Fortress+, remove --force-version and --versions.
  // Add --version and --json-output.
  // Update cmd/transport.bash_completion.sh accordingly.
  EXPECT_NE(std::string::npos, output.find("--force-version")) << output;
  EXPECT_NE(std::string::npos, output.find("--versions")) << output;

  // Flags in bash completion
  std::ifstream scriptFile(std::string(IGN_TRANSPORT_SOURCE_DIR) +
    "/src/cmd/transport.bash_completion.sh");
  std::string script((std::istreambuf_iterator<char>(scriptFile)),
      std::istreambuf_iterator<char>());

  EXPECT_NE(std::string::npos, script.find("--help")) << script;
  EXPECT_NE(std::string::npos, script.find("--version")) << script;
  EXPECT_NE(std::string::npos, script.find("--topic")) << script;
  EXPECT_NE(std::string::npos, script.find("--msgtype")) << script;
  EXPECT_NE(std::string::npos, script.find("--duration")) << script;
  EXPECT_NE(std::string::npos, script.find("--num")) << script;
  EXPECT_NE(std::string::npos, script.find("--list")) << script;
  EXPECT_NE(std::string::npos, script.find("--info")) << script;
  EXPECT_NE(std::string::npos, script.find("--echo")) << script;
  EXPECT_NE(std::string::npos, script.find("--pub")) << script;
  // NOTE: In Fortress+, remove --force-version and --versions.
  // Add --version and --json-output.
  // Update cmd/transport.bash_completion.sh accordingly.
  EXPECT_NE(std::string::npos, script.find("--force-version")) << script;
  EXPECT_NE(std::string::npos, script.find("--versions")) << script;
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
  // Save the current value of LD_LIBRARY_PATH.
  std::string value = "";
  ignition::transport::env("LD_LIBRARY_PATH", value);
  // Add the directory where ignition transport has been built.
  value = std::string(IGN_TEST_LIBRARY_PATH) + ":" + value;
  setenv("LD_LIBRARY_PATH", value.c_str(), 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
