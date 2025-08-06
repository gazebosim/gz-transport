/*
 * Copyright (C) 2025 Open Source Robotics Foundation
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
#include <gz/msgs/vector3d.pb.h>

#include <chrono>
#include <string>

#include "gz/transport/Node.hh"
#include "gz/transport/TransportTypes.hh"

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static std::string partition;  // NOLINT(*)
static std::string g_FQNPartition;  // NOLINT(*)
static const std::string g_topic = "/foo";  // NOLINT(*)

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber process there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcPubSubStats, PubSubTwoProcsThreeNodes)
{
  transport::Node node;
  auto pub = node.Advertise<msgs::Vector3d>(g_topic);
  EXPECT_TRUE(pub);

  // No subscribers yet.
  EXPECT_FALSE(pub.HasConnections());

  auto pi = gz::utils::Subprocess(
    {test_executables::kTwoProcsPubSubSubscriber, partition});

  msgs::Vector3d msg;
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
}

//////////////////////////////////////////////////
/// \brief This is the same as the last test, but we use PublishRaw(~) instead
/// of Publish(~).
TEST(twoProcPubSubStats, RawPubSubTwoProcsThreeNodes)
{
  transport::Node node;
  auto pub = node.Advertise<msgs::Vector3d>(g_topic);
  EXPECT_TRUE(pub);

  // No subscribers yet.
  EXPECT_FALSE(pub.HasConnections());

  auto pi = gz::utils::Subprocess(
    {test_executables::kTwoProcsPubSubSubscriber, partition});

  msgs::Vector3d msg;
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
    EXPECT_TRUE(pub.PublishRaw(msg.SerializeAsString(),
          std::string(msg.GetTypeName())));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();
  g_FQNPartition = std::string("/") + partition;

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);
  gz::utils::setenv("GZ_TRANSPORT_TOPIC_STATISTICS", "1");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
