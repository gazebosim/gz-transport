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
#ifdef _WIN32
  #include <filesystem>
#endif

#include "ignition/transport/Node.hh"
#include "msgs/vector3d.pb.h"
#include "gtest/gtest.h"
#include "ignition/transport/test_config.h"

using namespace ignition;

bool cbExecuted;
bool cb2Executed;
std::string topic = "/foo";
std::string data = "bar";

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const transport::msgs::Vector3d &_msg)
{
  EXPECT_DOUBLE_EQ(_msg.x(), 1.0);
  EXPECT_DOUBLE_EQ(_msg.y(), 2.0);
  EXPECT_DOUBLE_EQ(_msg.z(), 3.0);
  cbExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb2(const transport::msgs::Vector3d &_msg)
{
  EXPECT_DOUBLE_EQ(_msg.x(), 1.0);
  EXPECT_DOUBLE_EQ(_msg.y(), 2.0);
  EXPECT_DOUBLE_EQ(_msg.z(), 3.0);
  cb2Executed = true;
}

//////////////////////////////////////////////////
void runSubscriber()
{
  cbExecuted = false;
  cb2Executed = false;

  transport::Node node;
  transport::Node node2;

  EXPECT_TRUE(node.Subscribe(topic, cb));
  EXPECT_TRUE(node2.Subscribe(topic, cb2));

  int interval = 100;

  while (!cbExecuted || !cb2Executed)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    interval--;

    if (interval == 0)
      break;
  }

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);
  EXPECT_TRUE(cb2Executed);

  cb2Executed = false;
  EXPECT_TRUE(node.Unsubscribe(topic));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  cbExecuted = false;
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  // Check that the message was only received in node2
  EXPECT_FALSE(cbExecuted);
  EXPECT_TRUE(cb2Executed);
  cbExecuted = false;
  cb2Executed = false;
}

//////////////////////////////////////////////////
TEST(twoProcPubSub, PubSubTwoProcsTwoNodesSubscriber)
{
  runSubscriber();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Partition name has not be passed as argument" << std::endl;
    return -1;
  }

  // Set the partition name for this test.
  setenv("IGN_PARTITION", argv[1], 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
