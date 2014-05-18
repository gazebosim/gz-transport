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
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"

using namespace ignition;

bool cbExecuted;
std::string topic = "foo";
std::string data = "bar";

static void s_sleep(int msecs)
{
  struct timespec t;
  t.tv_sec = msecs / 1000;
  t.tv_nsec = (msecs % 1000) * 1000000;
  nanosleep(&t, nullptr);
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const robot_msgs::StringMsg &_msg)
{
  assert(_topic != "");

  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
}

//////////////////////////////////////////////////
void runPublisher()
{
  robot_msgs::StringMsg msg;
  msg.set_data(data);

  transport::Node node1;

  node1.Advertise(topic);
  s_sleep(500);
  node1.Publish(topic, msg);
  s_sleep(500);
}

//////////////////////////////////////////////////
void runSubscriber()
{
  cbExecuted = false;
  s_sleep(100);
  transport::Node node2;

  s_sleep(100);
  node2.Subscribe(topic, cb);
  s_sleep(500);

  // Check that the data was received
  cbExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubTwoProcesses)
{
  pid_t pid = fork();

  if (pid == 0)
    runPublisher();
  else
  {
    runSubscriber();

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
