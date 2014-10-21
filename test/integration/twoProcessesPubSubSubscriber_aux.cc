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
#include <string>
#include "ignition/transport/Node.hh"
#include "msg/vector3d.pb.h"
#include "gtest/gtest.h"
#include "ignition/transport/test_config.h"

#ifdef _WIN32
  #include <filesystem>
#endif

using namespace ignition;

bool cbExecuted;
bool cb2Executed;

std::string topic = "/foo";
std::string data = "bar";

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const transport::msgs::Vector3d &_msg)
{
  std::cout << "Callback1" << std::endl;
  EXPECT_EQ(_topic, topic);
  EXPECT_FLOAT_EQ(_msg.x(), 1.0);
  EXPECT_FLOAT_EQ(_msg.y(), 2.0);
  EXPECT_FLOAT_EQ(_msg.z(), 3.0);
  cbExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb2(const std::string &_topic, const transport::msgs::Vector3d &_msg)
{
  std::cout << "Callback2" << std::endl;
  EXPECT_EQ(_topic, topic);
  EXPECT_FLOAT_EQ(_msg.x(), 1.0);
  EXPECT_FLOAT_EQ(_msg.y(), 2.0);
  EXPECT_FLOAT_EQ(_msg.z(), 3.0);
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

  int interval = 100;

  while (!cbExecuted || !cb2Executed) {
   std::cout << "Waiting ... " << std::endl;
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   interval--;

   if (interval == 0)
     break;
  }

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


TEST(twoProcPubSub, PubSubTwoProcsTwoNodesSubscriber)
{
  runSubscriber();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
