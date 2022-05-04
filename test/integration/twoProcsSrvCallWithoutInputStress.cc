/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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
#include <ignition/msgs.hh>

#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "gz/transport/test_config.h"

using namespace ignition;

static std::string g_partition; // NOLINT(*)
static std::string g_topic = "/foo"; // NOLINT(*)

//////////////////////////////////////////////////
TEST(twoProcSrvCallWithoutInput, ThousandCalls)
{
  std::string responser_path = testing::portablePathUnion(
     IGN_TRANSPORT_TEST_DIR,
     "INTEGRATION_twoProcsSrvCallWithoutInputReplierInc_aux");

  testing::forkHandlerType pi = testing::forkAndRun(responser_path.c_str(),
    g_partition.c_str());

  ignition::msgs::Int32 response;
  bool result;
  unsigned int timeout = 1000;
  transport::Node node;

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  for (int i = 0; i < 15000; i++)
  {
    ASSERT_TRUE(node.Request(g_topic, timeout, response, result));

    // Check the service response.
    ASSERT_TRUE(result);
  }

  // Need to kill the responser node running on an external process.
  testing::killFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  g_partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", g_partition.c_str(), 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
