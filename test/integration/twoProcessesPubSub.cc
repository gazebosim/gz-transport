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
#include "msgs/int.pb.h"
#include "msgs/vector3d.pb.h"
#include "ignition/transport/test_config.h"

using namespace ignition;

std::string partition;
std::string topic = "/foo";
std::string data = "bar";
bool cbExecuted = false;
bool cbVectorExecuted = false;
int counter = 0;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  counter = 0;
  cbExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const transport::msgs::Int &/*_msg*/)
{
  cbExecuted = true;
  counter++;
}


//////////////////////////////////////////////////
/// \brief Callback for receiving Vector3d data.
void cbVector(const transport::msgs::Vector3d &/*_msg*/)
{
  cbVectorExecuted = true;
}

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

  testing::forkHandlerType pi = testing::forkAndRun(subscriberPath.c_str(),
    partition.c_str());

  transport::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  transport::Node node;
  EXPECT_TRUE(node.Advertise<transport::msgs::Vector3d>(topic));

  // Publish messages for a few seconds
  for (auto i = 0; i < 20; ++i)
  {
    EXPECT_TRUE(node.Publish(topic, msg));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check that a message is not received if the callback does not use
/// the advertised types.
TEST(twoProcPubSub, PubSubWrongTypesOnSubscription)
{
  std::string publisherPath = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;
  EXPECT_TRUE(node.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test spawns two subscribers on the same topic. One of the
/// subscribers has a wrong callback (types in the callback does not match the
/// advertised type). Check that only the good callback is executed.
TEST(NodeTest, PubSubWrongTypesTwoSubscribers)
{
  std::string publisherPath = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  transport::Node node1;
  transport::Node node2;
  EXPECT_TRUE(node1.Subscribe(topic, cb));
  EXPECT_TRUE(node2.Subscribe(topic, cbVector));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));

  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);
  EXPECT_TRUE(cbVectorExecuted);

  reset();

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
