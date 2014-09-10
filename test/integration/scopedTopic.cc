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

#include <sys/types.h>
#include <chrono>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "int.pb.h"

using namespace ignition;

bool cbExecuted;

std::string topic = "/foo";
int data = 5;

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const transport::msgs::Int &_msg)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
}

//////////////////////////////////////////////////
void runSubscriber()
{
  cbExecuted = false;
  transport::Node node;

  EXPECT_TRUE(node.Subscribe(topic, cb));

  int i = 0;
  while (i < 100 && !cbExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the message was not received because the scope was Process.
  EXPECT_FALSE(cbExecuted);
  cbExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Two different nodes, each one running in a different process. The
/// publisher advertises the topic as "process". This test checks that the topic
/// is not seen by the other node running in a different process.
TEST(ScopedTopicTest, ProcessTest)
{
  pid_t pid = fork();

  if (pid == 0)
    runSubscriber();
  else
  {
    transport::msgs::Int msg;
    msg.set_data(data);

    transport::Node node1;

    EXPECT_TRUE(node1.Advertise(topic, transport::Scope::Process));
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
