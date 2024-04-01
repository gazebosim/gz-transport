/*
 * Copyright (C) 2024 Open Source Robotics Foundation
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

#include <gtest/gtest.h>
#include <gz/msgs/int32.pb.h>

#include <gz/transport/Node.hh>

using namespace gz;

TEST(SameProcess, NewServiceAfterTimeout)
{
  transport::Node node;

  msgs::Int32 req;
  msgs::Int32 rep;
  bool result;
  unsigned int timeout = 1000;
  req.set_data(1);

  // Request a service with a timeout
  node.Request("/test_service", req, timeout, rep, result);
  // Expect the service to timeout.
  EXPECT_FALSE(result);

  // Sleep twice the timeout to ensure the service actually times out
  std::this_thread::sleep_for(std::chrono::milliseconds(timeout * 2));

  auto srvCb = std::function(
      [](const msgs::Int32 &, msgs::Int32 &_reply) -> bool
      {
        _reply.set_data(-1);
        ADD_FAILURE() << "Shouldn't have received service request after timout";
        return true;
      });

  node.Advertise("/test_service", srvCb);
}
