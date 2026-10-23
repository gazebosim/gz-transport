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

// A synchronous request whose reply type is msgs::Empty must receive
// the reply. The requests storage drops the handler of such a request
// right after the query is sent, so the reply has to reach the
// handler held by the waiting caller.

#include <gz/msgs/empty.pb.h>
#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <string>
#include <vector>

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gz/transport/Node.hh"
#include "gz/transport/WaitHelpers.hh"

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static std::string partition; // NOLINT(*)

//////////////////////////////////////////////////
TEST(zenohSrvCallEmptyReply, SyncRequest)
{
  gz::utils::Subprocess aux(
    std::vector<std::string>(
      {test_executables::kZenohSrvCallEmptyReplyAux, partition}));

  transport::Node node;
  ASSERT_TRUE(transport::waitForService(node, "/empty_reply_srv"))
    << "responder service never discovered";

  msgs::Int32 req;
  req.set_data(1);
  msgs::Empty rep;
  bool result = false;
  const unsigned int timeout = 5000u;

  const auto start = std::chrono::steady_clock::now();
  EXPECT_TRUE(node.Request("/empty_reply_srv", req, timeout, rep, result));
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start).count();

  EXPECT_TRUE(result);
  EXPECT_LT(elapsed, timeout);

  aux.Terminate();
  aux.Join();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name and the implementation for this process.
  gz::utils::setenv("GZ_PARTITION", partition);
  gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", "zenoh");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
