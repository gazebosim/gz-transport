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
#include <cstdlib>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "msg/int.pb.h"

#include "ignition/transport/test_config.h"

using namespace ignition;

std::string partition = "testPartition";
std::string ns = "";
std::string topic = "/foo";

//////////////////////////////////////////////////
TEST(twoProcSrvCall, ThousandCalls)
{
  std::string responser_path = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesSrvCallReplierIncreasing_aux");

  testing::forkHandlerType pi = testing::forkAndRun(responser_path.c_str());

  transport::msgs::Int req;
  transport::msgs::Int response;
  bool result;
  unsigned int timeout = 1000;
  transport::Node node(partition, ns);

  req.set_data(1);

  for (int i = 0; i < 1; i++)
  {
    EXPECT_TRUE(node.Request(topic, req, timeout, response, result));

    // Check the service response.
    EXPECT_TRUE(result);
    EXPECT_EQ(1, response.data());
  }

  // Need to kill the transport node
  testing::killFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Enable verbose mode.
  // Too much verbose generate tons of logs
  // disabling
  setenv("IGN_VERBOSE", "0", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
