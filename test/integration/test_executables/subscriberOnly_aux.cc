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
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Partition name has not be passed as argument" << std::endl;
    return -1;
  }

  // Set the partition name for this test.
  gz::utils::setenv("GZ_PARTITION", argv[1]);

  // Subscribe to a topic without any publisher and stay alive for a while,
  // giving the test process time to discover this subscription.
  transport::Node node;
  node.Subscribe(g_topic, cb);
  std::this_thread::sleep_for(std::chrono::seconds(10));
}
