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
TEST(twoProcPubSub, PubSubWrongTypesTwoSubscribers)
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
/// \brief This test spawns two nodes on different processes. One of the nodes
/// subscribes to a topic and the other advertises, publishes a message and
/// terminates. This test checks that the subscriber doesn't get affected by
/// the prompt termination of the publisher.
TEST(twoProcPubSub, FastPublisher)
{
  std::string publisherPath = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_fastPub_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;

  EXPECT_TRUE(node.Subscribe(topic, cbVector));
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test spawns two nodes on different processes. One of the nodes
/// advertises a topic and the other uses TopicList() for getting the list of
/// available topics.
TEST(twoProcPubSub, TopicList)
{
  std::string publisherPath = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;
  std::vector<std::string> topics;

  // We need some time for discovering the other node.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));

  auto start1 = std::chrono::steady_clock::now();
  node.TopicList(topics);
  auto end1 = std::chrono::steady_clock::now();
  ASSERT_EQ(topics.size(), 1u);
  EXPECT_EQ(topics.at(0), topic);
  topics.clear();

  // Time elapsed to get the first topic list
  auto elapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>
    (end1 - start1).count();

  auto start2 = std::chrono::steady_clock::now();
  node.TopicList(topics);
  auto end2 = std::chrono::steady_clock::now();
  EXPECT_EQ(topics.size(), 1u);
  EXPECT_EQ(topics.at(0), topic);

  // The first TopicList() call might block if the discovery is still
  // initializing (it may happen if we run this test alone).
  // However, the second call should never block.
  auto elapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>
    (end2 - start2).count();
  EXPECT_LE(elapsed2, elapsed1);

  EXPECT_LT(elapsed2, 2);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test spawns two nodes on different processes. One of the nodes
/// advertises a topic and the other uses TopicInfo() for getting information
/// about the topic.
TEST(twoProcPubSub, TopicInfo)
{
  std::string publisherPath = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;
  std::vector<transport::MessagePublisher> publishers;

  // We need some time for discovering the other node.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));

  EXPECT_FALSE(node.TopicInfo("@", publishers));
  EXPECT_EQ(publishers.size(), 0u);

  EXPECT_FALSE(node.TopicInfo("/bogus", publishers));
  EXPECT_EQ(publishers.size(), 0u);

  EXPECT_TRUE(node.TopicInfo("/foo", publishers));
  EXPECT_EQ(publishers.size(), 1u);
  EXPECT_EQ(publishers.front().MsgTypeName(),
            "ignition.transport.msgs.Vector3d");

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
