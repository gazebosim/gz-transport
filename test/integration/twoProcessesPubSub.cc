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
#include "msg/vector3d.pb.h"
#include "ignition/transport/test_config.h"

using namespace ignition;

std::string partition = "testPartition";
std::string ns = "";
std::string topic = "/foo";
std::string data = "bar";

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber processs there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcPubSub, PubSubTwoProcsTwoNodes)
{
  std::string subscriberPath = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesPubSubSubscriber_aux");

  testing::forkHandlerType pi = testing::forkAndRun(subscriberPath.c_str());

  transport::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  transport::Node node(partition, ns);

  EXPECT_TRUE(node.Advertise(topic));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(node.Publish(topic, msg));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(node.Publish(topic, msg));

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
