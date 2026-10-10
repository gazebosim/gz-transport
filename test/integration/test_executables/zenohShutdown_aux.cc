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

// Auxiliary binary for INTEGRATION_zenohShutdownStress.
// Opens a Node, declares one of each major Zenoh entity, then
// exits. The driving gtest runs this in a loop and fails if any
// run prints a Rust panic.

#include <gz/msgs/int32.pb.h>

#include <iostream>

#include "gz/transport/Node.hh"

namespace
{
  //////////////////////////////////////////////////
  /// \brief Service callback that always replies with 42.
  bool ServiceCallback(const gz::msgs::Int32 &/*_req*/,
                      gz::msgs::Int32 &_rep)
  {
    _rep.set_data(42);
    return true;
  }

  //////////////////////////////////////////////////
  /// \brief No-op topic callback.
  void TopicCallback(const gz::msgs::Int32 &/*_msg*/) {}
}

//////////////////////////////////////////////////
int main()
{
  gz::transport::Node node;

  auto pub = node.Advertise<gz::msgs::Int32>("/zenoh_shutdown/topic");
  (void)pub;

  node.Subscribe("/zenoh_shutdown/topic", TopicCallback);

  node.Advertise("/zenoh_shutdown/service", ServiceCallback);

  // Populate the Querier cache so shutdown exercises it too.
  gz::msgs::Int32 req;
  req.set_data(0);
  gz::msgs::Int32 rep;
  bool result = false;
  node.Request("/zenoh_shutdown/service", req, 100u, rep, result);

  return 0;
}
