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
static const std::string  g_topic     = "/topic";

// Global variables.
static std::string g_partition;

//////////////////////////////////////////////////
/// \brief Check 'ign topic -i' running the advertiser on a different process.
TEST(ignTest, cmdTopicInfo)
{
  transport::Node node;
  auto pub = node.Advertise<ignition::msgs::Int32>(g_topic);
  EXPECT_TRUE(pub);

  // Redirect stdout.
  std::stringstream buffer;
  auto old = std::cout.rdbuf(buffer.rdbuf());

  cmdTopicInfo(nullptr);

  // Verify that the stdout matches the expected output.
  EXPECT_EQ(buffer.str(), "");

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
