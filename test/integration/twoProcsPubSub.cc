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
#include <ignition/msgs.hh>

#include "gtest/gtest.h"
#include "ignition/transport/Node.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

static std::string partition;  // NOLINT(*)
static std::string g_FQNPartition;  // NOLINT(*)
static const std::string g_topic = "/foo";  // NOLINT(*)
static std::string data = "bar";  // NOLINT(*)
static bool cbExecuted = false;
static bool cbInfoExecuted = false;
static bool genericCbExecuted = false;
static bool cbVectorExecuted = false;
static bool cbRawExecuted = false;
static int counter = 0;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  cbExecuted = false;
  cbInfoExecuted = false;
  genericCbExecuted = false;
  cbVectorExecuted = false;
  cbRawExecuted = false;
  counter = 0;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const ignition::msgs::Int32 &/*_msg*/)
{
  cbExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cbInfo(const ignition::msgs::Int32 &_msg,
            const ignition::transport::MessageInfo &_info)
{
  EXPECT_EQ(_info.Topic(), g_topic);
  EXPECT_EQ(g_FQNPartition, _info.Partition());
  EXPECT_EQ(_msg.GetTypeName(), _info.Type());
  EXPECT_FALSE(_info.IntraProcess());
  cbInfoExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief A generic callback.
void genericCb(const transport::ProtoMsg &/*_msg*/)
{
  genericCbExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Callback for receiving Vector3d data.
void cbVector(const ignition::msgs::Vector3d &/*_msg*/)
{
  cbVectorExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
void cbRaw(const char * /*_msgData*/, const size_t /*_size*/,
           const ignition::transport::MessageInfo &/*_info*/)
{
  cbRawExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber process there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcPubSub, PubSubTwoProcsThreeNodes)
{
  transport::Node node;
  auto pub = node.Advertise<ignition::msgs::Vector3d>(g_topic);
  EXPECT_TRUE(pub);

  // No subscribers yet.
  EXPECT_FALSE(pub.HasConnections());

  std::string subscriberPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsPubSubSubscriber_aux");

  testing::forkHandlerType pi = testing::forkAndRun(subscriberPath.c_str(),
    partition.c_str());

  ignition::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Now, we should have subscribers.
  EXPECT_TRUE(pub.HasConnections());

  // Publish messages for a few seconds
  for (auto i = 0; i < 10; ++i)
  {
    EXPECT_TRUE(pub.Publish(msg));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This is the same as the last test, but we use PublishRaw(~) instead
/// of Publish(~).
TEST(twoProcPubSub, RawPubSubTwoProcsThreeNodes)
{
  transport::Node node;
  auto pub = node.Advertise<ignition::msgs::Vector3d>(g_topic);
  EXPECT_TRUE(pub);

  // No subscribers yet.
  EXPECT_FALSE(pub.HasConnections());

  std::string subscriberPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsPubSubSubscriber_aux");

  testing::forkHandlerType pi = testing::forkAndRun(subscriberPath.c_str(),
    partition.c_str());

  ignition::msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  unsigned int retries = 0u;

  while (!pub.HasConnections() && retries++ < 5u)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Now, we should have subscribers.
  EXPECT_LT(retries, 5u);

  // Publish messages for a few seconds
  for (auto i = 0; i < 10; ++i)
  {
    EXPECT_TRUE(pub.PublishRaw(msg.SerializeAsString(), msg.GetTypeName()));
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
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;
  EXPECT_TRUE(node.Subscribe(g_topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Same as above, but using a raw subscription.
TEST(twoProcPubSub, PubRawSubWrongTypesOnSubscription)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;
  EXPECT_TRUE(node.SubscribeRaw(g_topic, cbRaw,
                                ignition::msgs::Int32().GetTypeName()));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Check that the message was not received.
  EXPECT_FALSE(cbRawExecuted);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test spawns three subscribers on the same topic. The first
/// subscriber has a wrong callback (types in the callback does not match the
/// advertised type). The second subscriber uses the correct callback. The third
/// uses a generic callback. Check that only two of the callbacks are executed
/// (correct and generic).
TEST(twoProcPubSub, PubSubWrongTypesTwoSubscribers)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  transport::Node node1;
  transport::Node node2;
  transport::Node node3;
  EXPECT_TRUE(node1.Subscribe(g_topic, cb));
  EXPECT_TRUE(node2.Subscribe(g_topic, cbVector));
  EXPECT_TRUE(node3.Subscribe(g_topic, genericCb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));


  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);
  EXPECT_TRUE(cbVectorExecuted);
  EXPECT_TRUE(genericCbExecuted);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test spawns three raw subscribers on the same topic. The first
/// subscriber has the wrong type (the type specified to SubscribeRaw does not
/// match the advertised type). The second subscriber requests the correct type.
/// The third accepts the generic (default) type. Check that only two of the
/// callbacks are executed (correct and generic).
TEST(twoProcPubSub, PubSubWrongTypesTwoRawSubscribers)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  bool wrongRawCbExecuted = false;
  bool correctRawCbExecuted = false;
  bool genericRawCbExecuted = false;

  auto wrongCb = [&](const char *, const size_t /*_size*/,
                     const transport::MessageInfo &)
  {
    wrongRawCbExecuted = true;
  };

  auto correctCb = [&](const char *, const size_t /*_size*/,
                       const transport::MessageInfo &)
  {
    correctRawCbExecuted = true;
  };

  auto genericCb = [&](const char *, const size_t /*_size*/,
                       const transport::MessageInfo &)
  {
    genericRawCbExecuted = true;
  };

  transport::Node node1;
  transport::Node node2;
  transport::Node node3;
  EXPECT_TRUE(node1.SubscribeRaw(g_topic, wrongCb, "wrong.msg.type"));
  EXPECT_TRUE(node2.SubscribeRaw(g_topic, correctCb,
                                 msgs::Vector3d().GetTypeName()));
  EXPECT_TRUE(node3.SubscribeRaw(g_topic, genericCb));


  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));


  // Check that the message was not received.
  EXPECT_FALSE(wrongRawCbExecuted);
  EXPECT_TRUE(correctRawCbExecuted);
  EXPECT_TRUE(genericRawCbExecuted);

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
     IGN_TRANSPORT_TEST_DIR, "INTEGRATION_fastPub_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;

  EXPECT_TRUE(node.Subscribe(g_topic, cbVector));
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test creates one publisher and one subscriber on different
/// processes. The publisher publishes at higher frequency than the rate set
/// by the subscriber.
TEST(twoProcPubSub, SubThrottled)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR, "INTEGRATION_pub_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  transport::Node node;
  ignition::transport::SubscribeOptions opts;
  opts.SetMsgsPerSec(1u);
  EXPECT_TRUE(node.Subscribe(g_topic, cb, opts));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));

  // Node published 15 messages in ~1.5 sec. We should only receive 2 messages.
  EXPECT_LT(counter, 5);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test creates one publisher and one subscriber on different
/// processes. The publisher publishes at a throttled frequency.
TEST(twoProcPubSub, PubThrottled)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR, "INTEGRATION_pub_aux_throttled");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  transport::Node node;
  EXPECT_TRUE(node.Subscribe(g_topic, cb));

  // Wait for receive some messages.
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));

  // Node published 25 messages in ~2.5 sec. We should only receive 2 messages.
  EXPECT_LT(counter, 5);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief Check that a message is received after Advertise->Subscribe->Publish
/// using a callback that accepts message information.
TEST(twoProcPubSub, PubSubMessageInfo)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR, "INTEGRATION_twoProcsPublisher_aux");

  testing::forkHandlerType pi = testing::forkAndRun(publisherPath.c_str(),
    partition.c_str());

  reset();

  transport::Node node;
  EXPECT_TRUE(node.Subscribe(g_topic, cbInfo));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Check that the message was not received.
  EXPECT_FALSE(cbInfoExecuted);

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
/// \brief This test spawns two nodes on different processes. One of the nodes
/// advertises a topic and the other uses TopicList() for getting the list of
/// available topics.
TEST(twoProcPubSub, TopicList)
{
  std::string publisherPath = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR, "INTEGRATION_twoProcsPublisher_aux");

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
  EXPECT_EQ(topics.at(0), g_topic);
  topics.clear();

  // Time elapsed to get the first topic list
  auto elapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>
    (end1 - start1).count();

  auto start2 = std::chrono::steady_clock::now();
  node.TopicList(topics);
  auto end2 = std::chrono::steady_clock::now();
  EXPECT_EQ(topics.size(), 1u);
  EXPECT_EQ(topics.at(0), g_topic);

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
     IGN_TRANSPORT_TEST_DIR, "INTEGRATION_twoProcsPublisher_aux");

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
  EXPECT_EQ(publishers.front().MsgTypeName(), "ignition.msgs.Vector3d");

  reset();

  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();
  g_FQNPartition = std::string("/") + partition;

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);
  setenv("IGN_TRANSPORT_TOPIC_STATISTICS", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
