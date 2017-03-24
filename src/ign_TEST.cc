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

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <ignition/msgs.hh>
#include "ignition/transport/config.hh"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

/// \brief Check for empty topic.
TEST(ignTest, testEmptyTopic)
{
  //Node node;
  const char topic = "";
  const char *_topic = &topic;

  EXPECT_EQ("Invalid topic. Topic must not be empty.\n", cmdTopicInfo(const char *_topic))
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
