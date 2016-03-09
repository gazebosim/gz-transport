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
#include "ignition/transport/test_config.h"
#include "msgs/int.pb.h"

using namespace ignition;

std::string partition;
std::string topic = "/foo";

//////////////////////////////////////////////////
TEST(twoProcSrvCall, ThousandCalls)
{
  std::string responser_path = testing::portablePathUnion(
     PROJECT_BINARY_PATH,
     "test/integration/INTEGRATION_twoProcessesSrvCallReplierIncreasing_aux");

  testing::forkHandlerType pi = testing::forkAndRun(responser_path.c_str(),
    partition.c_str());

  transport::msgs::Int req;
  transport::msgs::Int response;
  bool result;
  unsigned int timeout = 1000;
  transport::Node node;

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  for (int i = 0; i < 15000; i++)
  {
    req.set_data(i);
    ASSERT_TRUE(node.Request(topic, req, timeout, response, result));

    // Check the service response.
    ASSERT_TRUE(result);
    EXPECT_EQ(i, response.data());
  }

  // Need to kill the responser node running on an external process.
  testing::killFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
