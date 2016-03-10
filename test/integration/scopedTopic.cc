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

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "msgs/int.pb.h"
#include "ignition/transport/test_config.h"

using namespace ignition;

std::string partition;
std::string topic = "/foo";
int data = 5;

//////////////////////////////////////////////////
/// \brief Two different nodes, each one running in a different process. The
/// publisher advertises the topic as "process". This test checks that the topic
/// is not seen by the other node running in a different process.
TEST(ScopedTopicTest, ProcessTest)
{
  std::string subscriber_path = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "/test/integration/INTEGRATION_scopedTopicSubscriber_aux");

  testing::forkHandlerType pi = testing::forkAndRun(subscriber_path.c_str(),
    partition.c_str());

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node;
  transport::AdvertiseOptions opts;
  opts.SetScope(transport::Scope_t::PROCESS);

  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic, opts));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(node.Publish(topic, msg));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(node.Publish(topic, msg));

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
