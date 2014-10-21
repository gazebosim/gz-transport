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
#include <chrono>
#include <cstdlib>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "msg/int.pb.h"

#include "ignition/transport/test_config.h"

using namespace ignition;

std::string topic = "/foo";
int data = 5;

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber processs there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcSrvCallSync2, SrvTwoProcs)
{
  std::string subscriber_path = testing::portable_path_union(
      PROJECT_BINARY_PATH, 
      "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");

  testing::fork_handler_t pi = testing::fork_and_run(subscriber_path.c_str());
   
  unsigned int timeout = 1000;
  transport::msgs::Int req;
  transport::msgs::Int rep;
  bool result;

  req.set_data(data);

  transport::Node node1;
  EXPECT_TRUE(node1.Request(topic, req, timeout, rep, result));
  EXPECT_EQ(req.data(), rep.data());
  EXPECT_TRUE(result);

  auto t1 = std::chrono::system_clock::now();
  EXPECT_FALSE(node1.Request("unknown_service", req, timeout, rep, result));
  auto t2 = std::chrono::system_clock::now();

  double elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was close to the timeout.
  EXPECT_NEAR(elapsed, timeout, 5.0);

  // Wait for the child process to return.
  testing::wait_and_cleanup_fork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
