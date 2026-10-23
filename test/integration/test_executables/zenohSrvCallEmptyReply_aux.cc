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

// Auxiliary binary for INTEGRATION_zenohSrvCallEmptyReply.
//
// Advertises a two way service whose reply type is msgs::Empty and
// serves it until the driving test terminates this process.

#include <gz/msgs/empty.pb.h>
#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <functional>
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

  gz::utils::setenv("GZ_PARTITION", argv[1]);
  gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", "zenoh");

  std::function<bool(const gz::msgs::Int32 &, gz::msgs::Empty &)> cb =
    [](const gz::msgs::Int32 &, gz::msgs::Empty &) -> bool
    {
      return true;
    };

  gz::transport::Node node;
  if (!node.Advertise("/empty_reply_srv", cb))
    return 1;

  // Bounded, in case the driving test dies without terminating us.
  std::this_thread::sleep_for(std::chrono::seconds(30));
  return 0;
}
