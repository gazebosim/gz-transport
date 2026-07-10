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

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static constexpr const char *g_topic = "/zenoh_pub_leak";

//////////////////////////////////////////////////
/// \brief Poll TopicList until _present matches, or timeout.
/// \param[in] _node Node used to query the topic list.
/// \param[in] _present Whether the topic is expected to be listed.
/// \param[in] _timeoutMs How long to keep polling.
/// \return True if the expected state was observed in time.
bool WaitForTopic(transport::Node &_node, bool _present,
                  int _timeoutMs)
{
  const auto deadline = std::chrono::steady_clock::now() +
    std::chrono::milliseconds(_timeoutMs);
  while (std::chrono::steady_clock::now() < deadline)
  {
    std::vector<std::string> topics;
    _node.TopicList(topics);
    const bool found =
      std::find(topics.begin(), topics.end(), g_topic) != topics.end();
    if (found == _present)
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

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

  transport::Node node;

  // The aux process advertises for ~3 s, destroys the publisher,
  // then lingers for ~8 s before exiting.
  gz::utils::Subprocess aux(
    std::vector<std::string>(
      {test_executables::kZenohPublisherLeakAux, partition}));

  // Phase 1: the topic must appear while the publisher is alive.
  EXPECT_TRUE(WaitForTopic(node, true, 3000))
    << "Topic was never discovered";

  // Phase 2: the topic must disappear once the remote publisher is
  // destroyed, well before the aux process exits.
  EXPECT_TRUE(WaitForTopic(node, false, 6000))
    << "Topic is still listed after the remote publisher was "
    << "destroyed: its liveliness token leaked";

  aux.Terminate();
  aux.Join();
  gz::utils::setenv("GZ_PARTITION", prevPartition);
}
