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

#include <gz/msgs/vector3d.pb.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_config.hh"

using namespace gz;

static const std::string g_topic = "/subscriber_only";  // NOLINT(*)

//////////////////////////////////////////////////
/// \brief A callback that is never expected to be executed because nobody
/// publishes on this topic.
void cb(const msgs::Vector3d &)
{
}

//////////////////////////////////////////////////
/// \brief Usage: subscriberOnly_aux <partition> [topic] [lifetimeSec]
/// [unsubscribeAfterSec] [readyFile].
/// Subscribe to a topic without any publisher and stay alive for
/// lifetimeSec, giving the test process time to discover this subscription.
/// If unsubscribeAfterSec is positive, unsubscribe after that time while
/// keeping the process alive until lifetimeSec. If readyFile is provided,
/// create that file right after subscribing, so that the test can
/// synchronize without waiting a fixed time.
int main(int argc, char **argv)
{
  if (argc < 2 || argc > 6)
  {
    std::cerr << "Partition name has not be passed as argument" << std::endl;
    return -1;
  }

  // Set the partition name for this test.
  gz::utils::setenv("GZ_PARTITION", argv[1]);

  const std::string topic = argc > 2 ? argv[2] : g_topic;
  const int lifetimeSec = argc > 3 ? std::stoi(argv[3]) : 10;
  const int unsubscribeAfterSec = argc > 4 ? std::stoi(argv[4]) : 0;
  const std::string readyFile = argc > 5 ? argv[5] : "";

  transport::Node node;
  node.Subscribe(topic, cb);

  if (!readyFile.empty())
    std::ofstream(readyFile) << "ready";

  int elapsedSec = 0;
  if (unsubscribeAfterSec > 0)
  {
    std::this_thread::sleep_for(std::chrono::seconds(unsubscribeAfterSec));
    node.Unsubscribe(topic);
    elapsedSec = unsubscribeAfterSec;
  }

  std::this_thread::sleep_for(std::chrono::seconds(lifetimeSec - elapsedSec));
}
