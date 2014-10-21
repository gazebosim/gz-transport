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

bool srvExecuted;
bool responseExecuted;

std::string topic = "/foo";
int data = 5;
int counter = 0;

//////////////////////////////////////////////////
/// \brief Service call response callback.
void response(const std::string &_topic, const transport::msgs::Int &_rep,
  bool _result)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_rep.data(), data);
  EXPECT_TRUE(_result);

  responseExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber processs there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcSrvCall, SrvTwoProcs)
{
  std::string subscriber_path = testing::portable_path_union(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");


  testing::fork_handler_t pi = testing::fork_and_run(subscriber_path.c_str());

  responseExecuted = false;
  counter = 0;
  transport::msgs::Int req;
  req.set_data(data);

  transport::Node node1;
  EXPECT_TRUE(node1.Request(topic, req, response));

  int i = 0;
  while (i < 100 && !responseExecuted)
  {
     std::this_thread::sleep_for(std::chrono::milliseconds(10));
     ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_EQ(counter, 1);

  // Make another request.
  responseExecuted = false;
  counter = 0;
  EXPECT_TRUE(node1.Request(topic, req, response));

  i = 0;
  while (i < 100 && !responseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_EQ(counter, 1);

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
