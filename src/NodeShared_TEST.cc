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

#include <gtest/gtest.h>
#include <gz/utils/Environment.hh>

#include "NodeSharedPrivate.hh"
#include "test_config.hh"

using ZenohConfigSource = gz::transport::NodeSharedPrivate::ZenohConfigSource;

//////////////////////////////////////////////////
/// \brief Test loading config from ZENOH_CONFIG env variable.
TEST(ZenohConfigTest, LoadFromEnvVariable)
{
  // Set ZENOH_CONFIG to point to our test config file
  ASSERT_TRUE(gz::utils::setenv("ZENOH_CONFIG", kZenohTestConfig));

  gz::transport::NodeSharedPrivate nodePrivate;
  ZenohConfigSource source;
  auto config = nodePrivate.ZenohConfig(source);

  // Verify the config source is from environment variable
  EXPECT_EQ(ZenohConfigSource::kFromEnvVariable, source);

  // The test config has id: "1234567890abcdef", verify it was loaded.
  // get() returns value in JSON format (with quotes)
  auto id = config.get("id");
  EXPECT_EQ("\"1234567890abcdef\"", id)
    << "Config should have id '1234567890abcdef' from test config file";

  // Clean up
  ASSERT_TRUE(gz::utils::unsetenv("ZENOH_CONFIG"));
}

//////////////////////////////////////////////////
/// \brief Test loading default config when no config files exist.
TEST(ZenohConfigTest, LoadDefaultConfig)
{
  // Ensure ZENOH_CONFIG is not set
  gz::utils::unsetenv("ZENOH_CONFIG");

  gz::transport::NodeSharedPrivate nodePrivate;
  ZenohConfigSource source;
  auto config = nodePrivate.ZenohConfig(source);

  // Verify the config source is default
  EXPECT_EQ(ZenohConfigSource::kDefault, source);

  // The test config has id: "1234567890abcdef", verify it was NOT loaded.
  // get() returns value in JSON format (with quotes)
  auto id = config.get("id");
  EXPECT_NE("\"1234567890abcdef\"", id)
    << "Config should NOT have id '1234567890abcdef' from test config file";
}

//////////////////////////////////////////////////
/// \brief Test that a nonexistent ZENOH_CONFIG path falls back to default
/// config and outputs a "not found" error message.
TEST(ZenohConfigTest, NonexistentConfigPath)
{
  // Set ZENOH_CONFIG to a nonexistent path
  ASSERT_TRUE(gz::utils::setenv("ZENOH_CONFIG",
    "/nonexistent/path/to/config.json5"));

  gz::transport::NodeSharedPrivate nodePrivate;
  ZenohConfigSource source;

  // Capture stderr to verify an error message is printed
  testing::internal::CaptureStderr();
  auto config = nodePrivate.ZenohConfig(source);
  std::string stderrOutput = testing::internal::GetCapturedStderr();

  // Should fall back to default config
  EXPECT_EQ(ZenohConfigSource::kDefault, source);

  // Should have printed a "not found" error message
  EXPECT_FALSE(stderrOutput.empty())
    << "Expected an error message on stderr for nonexistent config path";
  EXPECT_NE(std::string::npos, stderrOutput.find("not found"))
    << "Error message should contain 'not found'. Got: " << stderrOutput;

  // Clean up
  ASSERT_TRUE(gz::utils::unsetenv("ZENOH_CONFIG"));
}

//////////////////////////////////////////////////
/// \brief Test that an invalid ZENOH_CONFIG file (exists but has bad content)
/// falls back to default config and outputs a "parse" error message.
TEST(ZenohConfigTest, InvalidConfigContent)
{
  // Set ZENOH_CONFIG to a file with invalid Zenoh settings
  ASSERT_TRUE(gz::utils::setenv("ZENOH_CONFIG", kZenohInvalidConfig));

  gz::transport::NodeSharedPrivate nodePrivate;
  ZenohConfigSource source;

  // Capture stderr to verify an error message is printed
  testing::internal::CaptureStderr();
  auto config = nodePrivate.ZenohConfig(source);
  std::string stderrOutput = testing::internal::GetCapturedStderr();

  // Should fall back to default config
  EXPECT_EQ(ZenohConfigSource::kDefault, source);

  // Should have printed a "parse" error message
  EXPECT_FALSE(stderrOutput.empty())
    << "Expected an error message on stderr for invalid config content";
  EXPECT_NE(std::string::npos, stderrOutput.find("Failed to parse"))
    << "Error message should contain 'Failed to parse'. Got: " << stderrOutput;

  // Clean up
  ASSERT_TRUE(gz::utils::unsetenv("ZENOH_CONFIG"));
}

#endif  // HAVE_ZENOH
