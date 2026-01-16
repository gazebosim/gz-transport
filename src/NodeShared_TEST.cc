/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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

#include "gz/transport/config.hh"

#ifdef HAVE_ZENOH

#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>
#include <gz/utils/Environment.hh>

#include "gz/transport/Helpers.hh"
#include "NodeSharedPrivate.hh"
#include "test_config.hh"
#include "test_utils.hh"

namespace fs = std::filesystem;

//////////////////////////////////////////////////
/// \brief Test loading config from ZENOH_CONFIG env variable.
TEST(ZenohConfigTest, LoadFromEnvVariable)
{
  // Set ZENOH_CONFIG to point to our test config file (mode: "client")
  ASSERT_TRUE(gz::utils::setenv("ZENOH_CONFIG", kZenohTestConfig));

  gz::transport::NodeSharedPrivate nodePrivate;
  auto config = nodePrivate.ZenohConfig();

  // The test config has mode: "client", verify it was loaded
  // get() returns value in JSON format (with quotes)
  auto mode = config.get("mode");
  EXPECT_EQ("\"client\"", mode)
    << "Config should have mode 'client' from test config file";

  // Clean up
  ASSERT_TRUE(gz::utils::unsetenv("ZENOH_CONFIG"));
}

//////////////////////////////////////////////////
/// \brief Test loading config from user config path.
TEST(ZenohConfigTest, LoadFromUserConfig)
{
  // Ensure ZENOH_CONFIG is not set
  gz::utils::unsetenv("ZENOH_CONFIG");

  // Create a temporary directory to act as home
  fs::path tempHome = fs::temp_directory_path() /
    ("gz_test_home_" + testing::getRandomNumber());
  fs::create_directories(tempHome);

  // Set GZ_HOMEDIR to the temporary directory
  std::string originalHome;
  bool hadOriginalHome = gz::utils::env(GZ_HOMEDIR, originalHome);
  ASSERT_TRUE(gz::utils::setenv(GZ_HOMEDIR, tempHome.string()));

  // Create test user config with mode: "client"
  fs::path userConfigDir = tempHome / ".gz" / "transport";
  fs::path userConfigFile = userConfigDir / "gz_zenoh_session_config.json5";
  fs::create_directories(userConfigDir);
  {
    std::ofstream ofs(userConfigFile);
    ofs << "{ mode: \"client\" }";
  }

  gz::transport::NodeSharedPrivate nodePrivate;
  auto config = nodePrivate.ZenohConfig();

  // get() returns value in JSON format (with quotes)
  auto mode = config.get("mode");
  EXPECT_EQ("\"client\"", mode)
    << "Config should have mode 'client' from user config file";

  // Restore original GZ_HOMEDIR and clean up
  if (hadOriginalHome)
    gz::utils::setenv(GZ_HOMEDIR, originalHome);
  else
    gz::utils::unsetenv(GZ_HOMEDIR);
  fs::remove_all(tempHome);
}

//////////////////////////////////////////////////
/// \brief Test loading default config when no config files exist.
TEST(ZenohConfigTest, LoadDefaultConfig)
{
  // Ensure ZENOH_CONFIG is not set
  gz::utils::unsetenv("ZENOH_CONFIG");

  // Create an empty temporary directory to act as home
  fs::path tempHome = fs::temp_directory_path() /
    ("gz_test_home_" + testing::getRandomNumber());
  fs::create_directories(tempHome);

  // Set GZ_HOMEDIR to the temporary directory (no config files there)
  std::string originalHome;
  bool hadOriginalHome = gz::utils::env(GZ_HOMEDIR, originalHome);
  ASSERT_TRUE(gz::utils::setenv(GZ_HOMEDIR, tempHome.string()));

  gz::transport::NodeSharedPrivate nodePrivate;
  auto config = nodePrivate.ZenohConfig();

  // Default zenoh config has mode: "peer"
  // get() returns value in JSON format (with quotes)
  auto mode = config.get("mode");
  EXPECT_EQ("\"peer\"", mode)
    << "Default config should have mode 'peer'";

  // Restore original GZ_HOMEDIR and clean up
  if (hadOriginalHome)
    gz::utils::setenv(GZ_HOMEDIR, originalHome);
  else
    gz::utils::unsetenv(GZ_HOMEDIR);
  fs::remove_all(tempHome);
}

#endif  // HAVE_ZENOH
