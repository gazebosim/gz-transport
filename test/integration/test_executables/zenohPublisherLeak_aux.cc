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

// Auxiliary binary for INTEGRATION_zenohPublisherLeak.
// Advertises a topic, destroys the publisher handle while the
// process (and its Zenoh session) stays alive, then lingers. The
// driving gtest asserts that remote discovery sees the topic appear
// and then disappear while this process is still running, which
// only happens if the publisher teardown undeclares its liveliness
// token instead of leaking it.

#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Partition name has not been passed as argument"
              << std::endl;
    return -1;
  }

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", argv[1]);
  // Belt and braces: the driving test also exports this.
  gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", "zenoh");

  gz::transport::Node node;

  {
    auto pub = node.Advertise<gz::msgs::Int32>("/zenoh_pub_leak");
    if (!pub)
      return -1;

    // Give the test process time to observe the topic.
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));
  }
  // The publisher handle is destroyed here, but the process and its
  // Zenoh session stay alive. A correct teardown undeclares the
  // liveliness token now; a leak keeps the phantom entry until this
  // process exits.

  std::this_thread::sleep_for(std::chrono::milliseconds(12000));

  return 0;
}
