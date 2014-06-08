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

bool srvExecuted;
bool responseExecuted;

std::string topic = "foo";
std::string data = "bar";
int counter = 0;

//////////////////////////////////////////////////
/// \brief Provide a service.
bool srvEcho(const std::string &_topic, const robot_msgs::StringMsg &_req,
  robot_msgs::StringMsg &_rep)
{
  assert(_topic != "");
  srvExecuted = true;

  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());

  return true;
}

//////////////////////////////////////////////////
/// \brief Service call response callback.
void response(const std::string &_topic, const robot_msgs::StringMsg &_rep,
  bool _result)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_rep.data(), data);
  EXPECT_TRUE(_result);

  responseExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
void runReplier()
{
  srvExecuted = false;
  transport::Node node;
  node.Advertise(topic, srvEcho);

  int i = 0;
  while (i < 100 && !responseExecuted)
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
TEST(twoProcSrvCall, SrvTwoProcs)
{
  pid_t pid = fork();

  if (pid == 0)
    runReplier();
  else
  {
    responseExecuted = false;
    counter = 0;
    robot_msgs::StringMsg req;
    req.set_data(data);

    transport::Node node1;
    node1.Request(topic, req, response);

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
    node1.Request(topic, req, response);

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
