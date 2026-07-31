/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "gz/transport/Node.hh"
#include "gz/transport/WaitHelpers.hh"

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static std::string partition;  // NOLINT(*)

//////////////////////////////////////////////////
/// \brief This test spawns a process that only subscribes to a topic before
/// this process starts its discovery. The test verifies that the discovery
/// startup requests the existing subscribers, so the first TopicList() call
/// already includes the topic. This test needs its own process because the
/// discovery of a process only starts once.
TEST(topicListStartupSync, PreexistingSubscriberInFirstCall)
{
  // The remote subscriber exists before this process starts its discovery.
  const std::string readyFile = "subscriberOnly_" + partition + ".ready";
  std::filesystem::remove(readyFile);
  auto pi = testing::SubprocessJoinWrapper(
    {test_executables::kSubscriberOnly, partition, "/subscriber_only", "12",
     "0", readyFile});

  // Wait until the remote process is subscribed.
  ASSERT_TRUE(transport::waitUntil([&readyFile]
    {
      return std::filesystem::exists(readyFile);
    }));

  // The first transport node starts the discovery of this process, which
  // requests the current subscribers.
  transport::Node node;

  std::vector<std::string> topics;
  node.TopicList(topics);
  EXPECT_TRUE(std::find(topics.begin(), topics.end(), "/subscriber_only") !=
    topics.end());

  // The second call should finish as soon as all the known processes
  // report their subscribers, well below the internal timeout.
  topics.clear();
  auto start = std::chrono::steady_clock::now();
  node.TopicList(topics);
  auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_TRUE(std::find(topics.begin(), topics.end(), "/subscriber_only") !=
    topics.end());
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(
    elapsed).count(), 50);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
