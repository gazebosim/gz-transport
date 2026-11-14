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

//////////////////////////////////////////////////
/// \brief Test applying a single config override.
TEST(ZenohConfigTest, ConfigOverrideSingle)
{
  auto config = zenoh::Config::create_default();

  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, "scouting/multicast/enabled=false");

  EXPECT_EQ("false", config.get("scouting/multicast/enabled"));
}

//////////////////////////////////////////////////
/// \brief Test applying multiple config overrides separated by semicolons.
TEST(ZenohConfigTest, ConfigOverrideMultiple)
{
  auto config = zenoh::Config::create_default();

  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config,
    "scouting/multicast/enabled=false;"
    "listen/endpoints=[\"tcp/127.0.0.1:0\"]");

  EXPECT_EQ("false", config.get("scouting/multicast/enabled"));
  EXPECT_EQ("[\"tcp/127.0.0.1:0\"]", config.get("listen/endpoints"));
}

//////////////////////////////////////////////////
/// \brief Test that whitespace around keys and values is trimmed.
TEST(ZenohConfigTest, ConfigOverrideWhitespace)
{
  auto config = zenoh::Config::create_default();

  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, "  scouting/multicast/enabled  =  false  ");

  EXPECT_EQ("false", config.get("scouting/multicast/enabled"));
}

//////////////////////////////////////////////////
/// \brief Test that trailing semicolons are handled gracefully.
TEST(ZenohConfigTest, ConfigOverrideTrailingSemicolon)
{
  auto config = zenoh::Config::create_default();

  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, "scouting/multicast/enabled=false;");

  EXPECT_EQ("false", config.get("scouting/multicast/enabled"));
}

//////////////////////////////////////////////////
/// \brief Test that empty override string is a no-op.
TEST(ZenohConfigTest, ConfigOverrideEmpty)
{
  auto config = zenoh::Config::create_default();
  auto valBefore = config.get("scouting/multicast/enabled");

  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(config, "");

  EXPECT_EQ(valBefore, config.get("scouting/multicast/enabled"));
}

//////////////////////////////////////////////////
/// \brief Test that invalid keys produce an error on stderr but don't crash.
TEST(ZenohConfigTest, ConfigOverrideInvalidKey)
{
  auto config = zenoh::Config::create_default();

  testing::internal::CaptureStderr();
  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, "nonexistent/key/path=42");
  std::string stderrOutput = testing::internal::GetCapturedStderr();

  EXPECT_FALSE(stderrOutput.empty())
    << "Expected an error message for invalid config key";
  EXPECT_NE(std::string::npos, stderrOutput.find("failed to apply"))
    << "Error should contain 'failed to apply'. Got: " << stderrOutput;
}

//////////////////////////////////////////////////
/// \brief Test that bare values without '=' are silently skipped.
TEST(ZenohConfigTest, ConfigOverrideMalformedPair)
{
  auto config = zenoh::Config::create_default();

  // "noequals" has no '=', should be skipped; second pair should apply
  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, "noequals;scouting/multicast/enabled=false");

  EXPECT_EQ("false", config.get("scouting/multicast/enabled"));
}

//////////////////////////////////////////////////
/// \brief Test that values containing '=' are handled correctly.
/// The value ["tcp/host:7447=extra"] contains an '=' inside the JSON array.
TEST(ZenohConfigTest, ConfigOverrideEqualsInValue)
{
  auto config = zenoh::Config::create_default();

  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, "connect/endpoints=[\"tcp/127.0.0.1:7447\"]");

  EXPECT_EQ("[\"tcp/127.0.0.1:7447\"]", config.get("connect/endpoints"));
}

//////////////////////////////////////////////////
/// \brief Test that the env var integration path works end-to-end.
TEST(ZenohConfigTest, ConfigOverrideViaEnvVar)
{
  ASSERT_TRUE(gz::utils::setenv(
    "GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE",
    "scouting/multicast/enabled=false"));

  gz::transport::NodeSharedPrivate nodePrivate;
  ZenohConfigSource source;
  auto config = nodePrivate.ZenohConfig(source);

  const char *overrideEnv =
      std::getenv("GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE");
  ASSERT_NE(nullptr, overrideEnv);
  gz::transport::NodeSharedPrivate::ApplyZenohConfigOverrides(
    config, overrideEnv);

  EXPECT_EQ("false", config.get("scouting/multicast/enabled"));

  ASSERT_TRUE(gz::utils::unsetenv("GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE"));
}

#endif  // HAVE_ZENOH
