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

#include <robot_msgs/stringmsg.pb.h>
#include <sys/types.h>
#include <chrono>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"

using namespace ignition;

std::string topic = "foo";
std::string data = "bar";
bool srvExecuted = false;

//////////////////////////////////////////////////
/// \brief Provide a service.
void srvEcho(const std::string &_topic, const robot_msgs::StringMsg &_req,
  robot_msgs::StringMsg &_rep, bool &_result)
{
  assert(_topic != "");

  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());
  _result = true;

  srvExecuted = true;
}

//////////////////////////////////////////////////
void runReplier()
{
  srvExecuted = false;
  transport::Node node;
  node.Advertise(topic, srvEcho);

  int i = 0;
  while (i < 2000 && !srvExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call request was received.
  EXPECT_TRUE(srvExecuted);
}

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber processs there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcSrvCallSync, SrvTwoProcs)
{
  pid_t pid = fork();

  if (pid == 0)
    runReplier();
  else
  {
    robot_msgs::StringMsg req;
    robot_msgs::StringMsg rep;
    bool result;

    req.set_data(data);

    transport::Node node1;
    bool executed = node1.Request(topic, req, 5000, rep, result);

    EXPECT_TRUE(executed);
    EXPECT_EQ(req.data(), rep.data());
    EXPECT_TRUE(result);

    // Wait for the child process to return.
    int status;
    waitpid(pid, &status, 0);
  }
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
