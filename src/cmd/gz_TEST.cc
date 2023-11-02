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
#include <gz/msgs/int32.pb.h>
#include <gz/msgs/stringmsg.pb.h>
#include <gz/msgs/vector3d.pb.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <gz/utils/Environment.hh>
#include <gz/utils/ExtraTestMacros.hh>

#include "gtest/gtest.h"
#include "gz/transport/Node.hh"
#include "test_config.hh"

#ifdef _MSC_VER
#    define popen _popen
#    define pclose _pclose
#endif

using namespace gz;

static std::string g_partition; // NOLINT(*)
static std::string g_topicCBStr; // NOLINT(*)
static const std::string g_gzVersion("--force-version " + // NOLINT(*)
  std::string(GZ_VERSION_FULL));

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
bool srvEcho(const msgs::Int32 &_req, msgs::Int32 &_rep)
{
  _rep.set_data(_req.data());
  return true;
}

//////////////////////////////////////////////////
/// \brief Topic callback
void topicCB(const msgs::StringMsg &_msg)
{
  g_topicCBStr = _msg.data();
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic -l' running the advertiser on a different process.
TEST(gzTest, GZ_UTILS_TEST_DISABLED_ON_MAC(TopicList))
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    GZ_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'gz topic -l' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool topicFound = false;

  while (!topicFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(gz + " topic -l " + g_gzVersion);
    topicFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(topicFound);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic -i' running the advertiser on a different process.
TEST(gzTest, TopicInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    GZ_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'gz topic -i' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(gz + " topic -t /foo -i " + g_gzVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound) << "OUTPUT["
    << output << "] Size[" << output.size()
    << "]. Expected Size=50" << std::endl;
  EXPECT_TRUE(output.find("gz.msgs.Vector3d") != std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'gz service -l' running the advertiser on a different
/// process.
TEST(gzTest, ServiceList)
{
  // Launch a new responser process that advertises a service.
  std::string replier_path = testing::portablePathUnion(
    GZ_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    g_partition.c_str());

  // Check the 'gz service -l' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool serviceFound = false;

  while (!serviceFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(gz + " service -l " + g_gzVersion);
    serviceFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(serviceFound);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'gz service -i' running the advertiser on a different process.
TEST(gzTest, ServiceInfo)
{
  // Launch a new publisher process that advertises a topic.
  std::string replier_path = testing::portablePathUnion(
    GZ_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(replier_path.c_str(),
    g_partition.c_str());

  // Check the 'gz service -i' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(gz + " service -s /foo -i " + g_gzVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound);
  EXPECT_TRUE(output.find("gz.msgs.Int32") != std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic -l' running the advertiser on the same process.
TEST(gzTest, TopicListSameProc)
{
  transport::Node node;

  msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  auto pub = node.Advertise<msgs::Vector3d>("/foo");
  EXPECT_TRUE(pub);
  EXPECT_TRUE(pub.Publish(msg));

  // Check the 'gz topic -l' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool topicFound = false;

  while (!topicFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(gz + " topic -l " + g_gzVersion);
    topicFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(topicFound);
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic -i' running the advertiser on the same process.
TEST(gzTest, TopicInfoSameProc)
{
  transport::Node node;

  msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  auto pub = node.Advertise<msgs::Vector3d>("/foo");
  EXPECT_TRUE(pub);
  EXPECT_TRUE(pub.Publish(msg));

  // Check the 'gz topic -i' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(gz + " topic -t /foo -i " + g_gzVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound);
  EXPECT_TRUE(output.find("gz.msgs.Vector3d") != std::string::npos);
}

//////////////////////////////////////////////////
/// \brief Check 'gz service -l' running the advertiser on the same process.
TEST(gzTest, ServiceListSameProc)
{
  transport::Node node;
  EXPECT_TRUE(node.Advertise("/foo", srvEcho));

  // Check the 'gz service -l' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool serviceFound = false;

  while (!serviceFound && retries++ < 10u)
  {
    std::string output = custom_exec_str(gz + " service -l " + g_gzVersion);
    serviceFound = output == "/foo\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(serviceFound);
}

//////////////////////////////////////////////////
/// \brief Check 'gz service -i' running the advertiser on the same process.
TEST(gzTest, ServiceInfoSameProc)
{
  transport::Node node;
  EXPECT_TRUE(node.Advertise("/foo", srvEcho));

  // Check the 'gz service -i' command.
  std::string gz = std::string(GZ_PATH);

  unsigned int retries = 0u;
  bool infoFound = false;
  std::string output;

  while (!infoFound && retries++ < 10u)
  {
    output = custom_exec_str(gz + " service -s /foo -i " + g_gzVersion);
    infoFound = output.size() > 50u;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  EXPECT_TRUE(infoFound);
  EXPECT_TRUE(output.find("gz.msgs.Int32") != std::string::npos);
}


//////////////////////////////////////////////////
/// \brief Check 'gz topic -p' to send a message.
TEST(gzTest, TopicPublish)
{
  transport::Node node;
  g_topicCBStr = "bad_value";
  EXPECT_TRUE(node.Subscribe("/bar", topicCB));

  // Check the 'gz topic -p' command.
  std::string gz = std::string(GZ_PATH);
  std::string output;

  unsigned int retries = 0;
  while (g_topicCBStr != "good_value" && retries++ < 200u)
  {
    // Send on alternating retries
    if (retries % 2)
    {
      output = custom_exec_str(gz +
        " topic -t /bar -m gz_msgs.StringMsg -p 'data:\"good_value\"' " +
        g_gzVersion);
      EXPECT_TRUE(output.empty()) << output;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
  EXPECT_EQ(g_topicCBStr, "good_value");

  // Try to publish a message not included in Gazebo Messages.
  std::string error = "Unable to create message of type";
  output = custom_exec_str(gz +
      " topic -t /bar -m gz_msgs.__bad_msg_type -p 'data:\"good_value\"' " +
      g_gzVersion);
  EXPECT_EQ(output.compare(0, error.size(), error), 0);

  // Try to publish using an incorrect topic name.
  error = "Topic [/] is not valid";
  output = custom_exec_str(gz +
      " topic -t / -m gz_msgs.StringMsg -p 'data:\"good_value\"' "+
      g_gzVersion);
  EXPECT_EQ(output.compare(0, error.size(), error), 0) << output;

  // Try to publish using an incorrect number of arguments.
  error = "The following argument was not expected: wrong_topic";
  output = custom_exec_str(gz +
      " topic -t / wrong_topic -m gz_msgs.StringMsg -p 'data:\"good_value\"' "+
      g_gzVersion);
  EXPECT_EQ(output.compare(0, error.size(), error), 0) << output;
}

//////////////////////////////////////////////////
/// \brief Check 'gz service -r' to request a service.
TEST(gzTest, ServiceRequest)
{
  transport::Node node;

  // Advertise a service.
  std::string service = "/echo";
  std::string value = "10";
  EXPECT_TRUE(node.Advertise(service, srvEcho));

  msgs::Int32 msg;
  msg.set_data(10);

  // Check the 'gz service -r' command.
  std::string gz = std::string(GZ_PATH);
  std::string output = custom_exec_str(gz +
      " service -s " + service + " --reqtype gz_msgs.Int32 " +
      "--reptype gz_msgs.Int32 --timeout 1000 " +
      "--req 'data: " + value + "' " + g_gzVersion);

  ASSERT_EQ(output, "data: " + value + "\n\n");
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic -e' running the publisher on a separate process.
TEST(gzTest, TopicEcho)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    GZ_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'gz topic -e' command.
  std::string gz = std::string(GZ_PATH);
  std::string output = custom_exec_str(
    gz + " topic -e -t /foo -d 1.5 " + g_gzVersion);

  EXPECT_TRUE(output.find("x: 1") != std::string::npos);
  EXPECT_TRUE(output.find("y: 2") != std::string::npos);
  EXPECT_TRUE(output.find("z: 3") != std::string::npos);

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic -e -n 2' running the publisher on a separate
/// process.
TEST(gzTest, TopicEchoNum)
{
  // Launch a new publisher process that advertises a topic.
  std::string publisher_path = testing::portablePathUnion(
    GZ_TRANSPORT_TEST_DIR,
    "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisher_path.c_str(),
    g_partition.c_str());

  // Check the 'gz topic -e -n' command.
  std::string gz = std::string(GZ_PATH);
  std::string output = custom_exec_str(
    gz + " topic -e -t /foo -n 2 " + g_gzVersion);

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
/// \brief Check 'gz service --help' message and bash completion script for
/// consistent flags
TEST(gzTest, ServiceHelpVsCompletionFlags)
{
  // Flags in help message
  std::string helpOutput = custom_exec_str("gz service --help");

  // Call the output function in the bash completion script
  std::filesystem::path scriptPath = PROJECT_SOURCE_DIR;
  scriptPath = scriptPath / "src" / "cmd" / "transport.bash_completion.sh";

  // Equivalent to:
  // sh -c "bash -c \". /path/to/transport.bash_completion.sh;
  // _gz_service_flags\""
  std::string cmd = "bash -c \". " + scriptPath.string() +
    "; _gz_service_flags\"";
  std::string scriptOutput = custom_exec_str(cmd);

  // Tokenize script output
  std::istringstream iss(scriptOutput);
  std::vector<std::string> flags((std::istream_iterator<std::string>(iss)),
    std::istream_iterator<std::string>());

  EXPECT_GT(flags.size(), 0u);

  // Match each flag in script output with help message
  for (const auto &flag : flags)
  {
    EXPECT_NE(std::string::npos, helpOutput.find(flag)) << helpOutput;
  }
}

//////////////////////////////////////////////////
/// \brief Check 'gz topic --help' message and bash completion script for
/// consistent flags
TEST(gzTest, TopicHelpVsCompletionFlags)
{
  // Flags in help message
  std::string helpOutput = custom_exec_str("gz topic --help");

  // Call the output function in the bash completion script
  std::filesystem::path scriptPath = PROJECT_SOURCE_DIR;
  scriptPath = scriptPath / "src" / "cmd" / "transport.bash_completion.sh";

  // Equivalent to:
  // sh -c "bash -c \". /path/to/transport.bash_completion.sh;
  // _gz_topic_flags\""
  std::string cmd = "bash -c \". " + scriptPath.string() +
    "; _gz_topic_flags\"";
  std::string scriptOutput = custom_exec_str(cmd);

  // Tokenize script output
  std::istringstream iss(scriptOutput);
  std::vector<std::string> flags((std::istream_iterator<std::string>(iss)),
    std::istream_iterator<std::string>());

  EXPECT_GT(flags.size(), 0u);

  // Match each flag in script output with help message
  for (const auto &flag : flags)
  {
    EXPECT_NE(std::string::npos, helpOutput.find(flag)) << helpOutput;
  }
}

/////////////////////////////////////////////////
/// Main
int main(int argc, char **argv)
{
  // Get a random partition name.
  g_partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", g_partition);

  // Make sure that we load the library recently built and not the one installed
  // in your system.
  // Save the current value of LD_LIBRARY_PATH.
  std::string value = "";
  transport::env("LD_LIBRARY_PATH", value);
  // Add the directory where Gazebo Transport has been built.
  value = std::string(GZ_TEST_LIBRARY_PATH) + ":" + value;
  gz::utils::setenv("LD_LIBRARY_PATH", value);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
