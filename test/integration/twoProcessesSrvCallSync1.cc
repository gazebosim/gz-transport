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

#include <ignition/msgs.hh>
#include <sys/types.h>
#include <chrono>
#include <cstdlib>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"

using namespace ignition;

std::string topic = "/foo";
std::string data = "bar";

//////////////////////////////////////////////////
/// \brief Provide a service.
void srvEcho(const std::string &_topic, const ignition::msgs::StringMsg &_req,
  ignition::msgs::StringMsg &_rep, bool &_result)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
void runReplier()
{
  // srvExecuted = false;
  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  int i = 0;
  while (i < 100)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }
}

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber processs there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcSrvCallSync1, SrvTwoProcs)
{
  pid_t pid = fork();

  if (pid == 0)
    runReplier();
  else
  {
    unsigned int timeout = 500;
    ignition::msgs::StringMsg req;
    ignition::msgs::StringMsg rep;
    bool result;

    req.set_data(data);

    transport::Node node1;

    // Make sure that the address of the service call provider is known.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(node1.Request(topic, req, timeout, rep, result));
    EXPECT_EQ(req.data(), rep.data());
    EXPECT_TRUE(result);

    auto t1 = std::chrono::system_clock::now();
    EXPECT_FALSE(node1.Request("unknown_service", req, timeout, rep, result));
    auto t2 = std::chrono::system_clock::now();

    double elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    // Check if the elapsed time was close to the timeout.
    EXPECT_NEAR(elapsed, timeout, 1.0);

    // Wait for the child process to return.
    int status;
    waitpid(pid, &status, 0);
  }
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
