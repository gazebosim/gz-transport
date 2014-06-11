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

bool cbExecuted;
bool cb2Executed;

std::string topic = "/foo";
std::string data = "bar";

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const robot_msgs::StringMsg &_msg)
{
  assert(_topic != "");

  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb2(const std::string &_topic, const robot_msgs::StringMsg &_msg)
{
  assert(_topic != "");

  EXPECT_EQ(_msg.data(), data);
  cb2Executed = true;
}

//////////////////////////////////////////////////
void runSubscriber()
{
  cbExecuted = false;
  cb2Executed = false;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  transport::Node node;
  transport::Node node2;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(node.Subscribe(topic, cb));
  EXPECT_TRUE(node2.Subscribe(topic, cb2));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);
  EXPECT_TRUE(cb2Executed);
  cbExecuted = false;
  cb2Executed = false;

  EXPECT_TRUE(node.Unsubscribe(topic));
  std::this_thread::sleep_for(std::chrono::milliseconds(600));

  // Check that the message was only received in node3.
  EXPECT_FALSE(cbExecuted);
  EXPECT_TRUE(cb2Executed);
  cbExecuted = false;
  cb2Executed = false;
}

//////////////////////////////////////////////////
/// \brief Three different nodes running in two different processes. In the
/// subscriber processs there are two nodes. Both should receive the message.
/// After some time one of them unsubscribe. After that check that only one
/// node receives the message.
TEST(twoProcPubSub, PubSubTwoProcsTwoNodes)
{
  pid_t pid = fork();

  if (pid == 0)
    runSubscriber();
  else
  {
    robot_msgs::StringMsg msg;
    msg.set_data(data);

    transport::Node node1;

    EXPECT_TRUE(node1.Advertise(topic));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(node1.Publish(topic, msg));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(node1.Publish(topic, msg));

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
