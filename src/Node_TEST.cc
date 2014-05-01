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
#include <memory>
#include <string>
#include <thread>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"

using namespace ignition;

bool cbExecuted;
bool cb2Executed;
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
void cb(const std::string &_topic,
        const std::shared_ptr<const robot_msgs::StringMsg> &_msgPtr)
{
  assert(_topic != "");

  EXPECT_EQ(_msgPtr->data(), data);
  cbExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb2(const std::string &_topic,
         const std::shared_ptr<const robot_msgs::StringMsg> &_msgPtr)
{
  assert(_topic != "");

  EXPECT_EQ(_msgPtr->data(), data);
  cb2Executed = true;
}

//////////////////////////////////////////////////
void CreateSubscriber()
{
  transport::Node node;
  EXPECT_EQ(node.Subscribe(topic, cb), 0);
  while (!cbExecuted)
    s_sleep(100);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubWithoutAdvertise)
{
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  // Subscribe to topic1
  transport::Node node;

  // Publish some data on topic1 without advertising it first
  EXPECT_NE(node.Publish(topic, msgPtr), 0);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThread)
{
  cbExecuted = false;
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  transport::Node node;

  // Advertise topic1
  EXPECT_EQ(node.Advertise(topic), 0);

  // Subscribe to topic1
  EXPECT_EQ(node.Subscribe(topic, cb), 0);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node.Publish(topic, msgPtr), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Publish a second message on topic1
  EXPECT_EQ(node.Publish(topic, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Unadvertise topic1 and publish a third message
  node.UnAdvertise(topic);
  EXPECT_NE(node.Publish(topic, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cbExecuted);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThreadLocal)
{
  cbExecuted = false;
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());

  msgPtr->set_data(data);

  transport::Node node;

  // Advertise topic1
  EXPECT_EQ(node.Advertise(topic), 0);

  // Subscribe to topic1
  node.Subscribe(topic, cb);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node.Publish(topic, msgPtr), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Publish a second message on topic1
  EXPECT_EQ(node.Publish(topic, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Unadvertise topic1 and publish a third message
  node.UnAdvertise(topic);
  EXPECT_NE(node.Publish(topic, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cbExecuted);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameProcess)
{
  cbExecuted = false;
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  // Create the transport node
  transport::Node node;
  EXPECT_EQ(node.Advertise(topic), 0);
  s_sleep(100);

  // Subscribe to topic1 in a different thread
  std::thread subscribeThread(CreateSubscriber);
  s_sleep(100);

  // Advertise and publish a msg on topic1
  EXPECT_EQ(node.Publish(topic, msgPtr), 0);
  s_sleep(100);

  subscribeThread.join();

  // Check that the data was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, TwoSubscribersSameThread)
{
  cbExecuted = false;
  cb2Executed = false;
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  transport::Node node1;
  transport::Node node2;

  // Advertise topic
  EXPECT_EQ(node1.Advertise(topic), 0);
  s_sleep(100);

  // Subscribe to topic from node1
  EXPECT_EQ(node1.Subscribe(topic, cb), 0);
  s_sleep(100);

  // Subscribe to topic from node2
  EXPECT_EQ(node2.Subscribe(topic, cb2), 0);
  s_sleep(100);

  // Publish a msg on topic
  EXPECT_EQ(node1.Publish(topic, msgPtr), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Check that the msg was received
  EXPECT_TRUE(cb2Executed);
  cb2Executed = false;

  // Unadvertise topic and publish a third message
  node1.UnAdvertise(topic);
  EXPECT_NE(node1.Publish(topic, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(cb2Executed);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
