/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#include "gtest/gtest.h"

#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

static std::string partition; // NOLINT(*)
static std::string g_FQNPartition; // NOLINT(*)

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();
  g_FQNPartition = std::string("/") + partition;

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
