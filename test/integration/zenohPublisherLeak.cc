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
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gz/transport/Node.hh"
#include "gz/transport/WaitHelpers.hh"

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static constexpr const char *g_topic = "/zenoh_pub_leak";

//////////////////////////////////////////////////
/// \brief A remote process advertises a topic and then destroys the
/// publisher while its process stays alive. This test asserts that
/// discovery in this process forgets the topic at that point. With
/// a leaked liveliness token the phantom publisher only clears when
/// the remote process exits, so the disappearance check fails while
/// the auxiliary process is still alive.
TEST(zenohPublisherLeak, DiscoveryForgetsDestroyedPublisher)
{
  std::string prevPartition;
  gz::utils::env("GZ_PARTITION", prevPartition);
  const std::string partition = testing::getRandomNumber();
  gz::utils::setenv("GZ_PARTITION", partition);

  // This test validates the Zenoh liveliness teardown specifically;
  // force the implementation so a zeromq default build cannot make
  // it pass vacuously. The aux subprocess inherits the environment.
  std::string prevImpl;
  gz::utils::env("GZ_TRANSPORT_IMPLEMENTATION", prevImpl);
  gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", "zenoh");

  transport::Node node;

  // The aux process advertises for ~6 s, destroys the publisher,
  // then lingers for ~12 s before exiting.
  gz::utils::Subprocess aux(
    std::vector<std::string>(
      {test_executables::kZenohPublisherLeakAux, partition}));

  // Phase 1: the topic must appear while the publisher is alive.
  // The budget covers subprocess spawn plus the aux Node
  // constructor's bounded cold-start waits.
  EXPECT_TRUE(transport::waitForTopic(node, g_topic,
    std::chrono::milliseconds(8000))) << "Topic was never discovered";

  // Phase 2: the topic must disappear once the remote publisher is
  // destroyed, well before the aux process exits.
  EXPECT_TRUE(transport::waitUntil([&node]
    {
      std::vector<std::string> topics;
      node.TopicList(topics);
      return std::find(topics.begin(), topics.end(), g_topic) ==
        topics.end();
    }, std::chrono::milliseconds(10000)))
    << "Topic is still listed after the remote publisher was "
    << "destroyed: its liveliness token leaked";

  aux.Terminate();
  aux.Join();
  gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", prevImpl);
  gz::utils::setenv("GZ_PARTITION", prevPartition);
}
