/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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
#include <cstdlib>
#include <string>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gtest/gtest.h"

#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static std::string partition; // NOLINT(*)
static std::string g_topic = "/foo"; // NOLINT(*)
static int data = 5;

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and a service requester. The
/// synchronous requester uses a wrong service's name. The test should verify
/// that the service call does not succeed and the elapsed time was close to
/// the timeout.
TEST(twoProcSrvCallSync1, SrvTwoProcs)
{
  auto pi = gz::utils::Subprocess(
    {test_executables::kTwoProcsSrvCallReplier, partition});

  int64_t timeout = 500;
  msgs::Int32 req;
  msgs::Int32 rep;
  bool result;

  req.set_data(data);

  transport::Node node;

  ASSERT_TRUE(transport::waitForService(node, g_topic))
      << "Service not discovered within timeout";

  ASSERT_TRUE(node.Request(g_topic, req,
      static_cast<unsigned int>(timeout), rep, result));
  EXPECT_EQ(req.data(), rep.data());
  EXPECT_TRUE(result);

  auto t1 = std::chrono::steady_clock::now();
  EXPECT_FALSE(node.Request("unknown_service", req,
      static_cast<unsigned int>(timeout), rep, result));
  auto t2 = std::chrono::steady_clock::now();

  int64_t elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was at least the timeout value.
  // Under heavy system load, the actual elapsed time may be significantly
  // longer than the timeout, so we only check the lower bound.
  EXPECT_GE(elapsed, timeout - 100);

  pi.Terminate();
  pi.Join();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);

  // Enable verbose mode.
  gz::utils::setenv("GZ_VERBOSE", "1");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
