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

#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_config.hh"

using namespace gz;

static const std::string g_topic = "/bar";  // NOLINT(*)

//////////////////////////////////////////////////
/// \brief Usage: fastPubLoop_aux <partition> [lifetimeSec].
/// Publish an Int32 message on /bar every millisecond during lifetimeSec,
/// so that a subscriber in another process keeps receiving messages.
int main(int argc, char **argv)
{
  if (argc < 2 || argc > 3)
  {
    std::cerr << "Partition name has not be passed as argument" << std::endl;
    return -1;
  }

  // Set the partition name for this test.
  gz::utils::setenv("GZ_PARTITION", argv[1]);

  const int lifetimeSec = argc > 2 ? std::stoi(argv[2]) : 10;

  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>(g_topic);

  msgs::Int32 msg;
  msg.set_data(1);
  const auto end =
    std::chrono::steady_clock::now() + std::chrono::seconds(lifetimeSec);
  while (std::chrono::steady_clock::now() < end)
  {
    pub.Publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
