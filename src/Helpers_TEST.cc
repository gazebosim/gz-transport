/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/test_config.h"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the env() function.
TEST(HelpersTest, env)
{
  // Create a random string.
  std::string name = testing::getRandomNumber();

  // Check that an unknown environment variable returns false.
  std::string value;
  EXPECT_FALSE(transport::env(name, value));

  // Create a random environment variable and give it its name as value.
  setenv(name.c_str(), name.c_str(), 1);

  // Check that we find the environment variable and the value is correct.
  EXPECT_TRUE(transport::env(name, value));
  EXPECT_EQ(name, value);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
