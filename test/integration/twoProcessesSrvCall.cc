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
#include "ignition/transport/TopicUtils.hh"
#include "gtest/gtest.h"
#include "ignition/transport/test_config.h"
#include "msgs/int.pb.h"

using namespace ignition;

bool srvExecuted;
bool responseExecuted;

std::string partition;
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
/// \brief Two different nodes running in two different processes. One node
/// advertises a service and the other requests a few service calls.
TEST(twoProcSrvCall, SrvTwoProcs)
{
  std::string responser_path = testing::portablePathUnion(
    PROJECT_BINARY_PATH,
    "test/integration/INTEGRATION_twoProcessesSrvCallReplier_aux");

  testing::forkHandlerType pi = testing::forkAndRun(responser_path.c_str(),
    partition.c_str());

  responseExecuted = false;
  counter = 0;
  transport::msgs::Int req;
  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Request(topic, req, response));

  int i = 0;
  while (i < 300 && !responseExecuted)
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
  EXPECT_TRUE(node.Request(topic, req, response));

  i = 0;
  while (i < 300 && !responseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_EQ(counter, 1);

  EXPECT_TRUE(node.UnadvertiseSrv(topic));
  EXPECT_TRUE(node.AdvertisedServices().empty());

  // Wait for the child process to return.
  testing::waitAndCleanupFork(pi);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  // Enable verbose mode.
  // setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
