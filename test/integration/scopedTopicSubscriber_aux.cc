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
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "msg/int.pb.h"
#include "test_config.h"

#ifdef _WIN32
  #include <filesystem>
#endif

using namespace ignition;

bool cbExecuted;

std::string topic = "/foo";
int data = 5;

//////////////////////////////////////////////////
/// \brief Two different nodes, each one running in a different process. The
/// publisher advertises the topic as "process". This test checks that the topic
/// is not seen by the other node running in a different process.
TEST(ScopedTopicTest, ProcessTest)
{

#ifdef _WIN32
  std::path subscriber_path = PROJECT_BINARY_PATH;
  subscriber_path /= "test/integration/INTEGRATION_subscriber_app";
#else
  std::string subscriber_path = PROJECT_BINARY_PATH;
  subscriber_path += "/test/integration/INTEGRATION_subscriber_app";
#endif

  ASSERT_EQ(system(subscriber_path.c_str()),0) << "Failed to run system call";
  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node1;

  EXPECT_TRUE(node1.Advertise(topic, transport::Scope::Process));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(node1.Publish(topic, msg));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(node1.Publish(topic, msg));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
