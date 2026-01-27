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

//////////////////////////////////////////////////
/// \brief Test loading config from ZENOH_CONFIG env variable.
TEST(ZenohConfigTest, LoadFromEnvVariable)
{
  // Set ZENOH_CONFIG to point to our test config file
  ASSERT_TRUE(gz::utils::setenv("ZENOH_CONFIG", kZenohTestConfig));

  gz::transport::NodeSharedPrivate nodePrivate;
  auto config = nodePrivate.ZenohConfig();

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
  auto config = nodePrivate.ZenohConfig();

  // The test config has id: "1234567890abcdef", verify it was NOT loaded.
  // get() returns value in JSON format (with quotes)
  auto id = config.get("id");
  EXPECT_NE("\"1234567890abcdef\"", id)
    << "Config should NOT have id '1234567890abcdef' from test config file";
}

#endif  // HAVE_ZENOH
