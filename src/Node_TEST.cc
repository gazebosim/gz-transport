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

#include <limits.h>
#include <uuid/uuid.h>
#include <robot_msgs/stringmsg.pb.h>
#include <memory>
#include <string>
#include <thread>
#include "ignition/transport/Node.hh"
#include "ignition/transport/zhelpers.hpp"
#include "gtest/gtest.h"

using namespace ignition;

bool cb1Executed;
bool cb2Executed;

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const std::string &_data)
{
  assert(_topic != "");

  robot_msgs::StringMsg str;
  str.ParseFromString(_data);
  EXPECT_EQ(str.data(), "someData");
  cb1Executed = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb2(const std::string &_topic, const std::string &_data)
{
  assert(_topic != "");

  robot_msgs::StringMsg str;
  str.ParseFromString(_data);
  EXPECT_EQ(str.data(), "someData");
  cb2Executed = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb3(const std::string &_topic,
         const std::shared_ptr<robot_msgs::StringMsg> &_msgPtr)
{
  assert(_topic != "");

  EXPECT_EQ(_msgPtr->data(), "someData");
  cb2Executed = true;
}

//////////////////////////////////////////////////
void CreateSubscriber()
{
  std::string topic1 = "foo";
  bool verbose = false;
  transport::Node node(verbose);
  EXPECT_EQ(node.Subscribe(topic1, cb), 0);
  while (!cb1Executed)
    sleep(1);
}

//////////////////////////////////////////////////
/*TEST(DiscZmqTest, PubWithoutAdvertise)
{
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  // Subscribe to topic1
  transport::Node node(verbose);

  // Publish some data on topic1 without advertising it first
  EXPECT_NE(node.Publish(topic1, msgPtr), 0);
}*/

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThread)
{
  cb1Executed = false;
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  transport::Node node(verbose);

  // Advertise topic1
  EXPECT_EQ(node.Advertise(topic1), 0);

  // Subscribe to topic1
  EXPECT_EQ(node.Subscribe(topic1, cb), 0);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cb1Executed);
  cb1Executed = false;

  // Publish a second message on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cb1Executed);
  cb1Executed = false;

  // Unadvertise topic1 and publish a third message
  node.UnAdvertise(topic1);
  EXPECT_NE(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cb1Executed);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThreadLocal)
{
  cb1Executed = false;
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  transport::Node node(verbose);

  // Advertise topic1
  EXPECT_EQ(node.Advertise(topic1), 0);

  // Subscribe to topic1
  EXPECT_EQ(node.SubscribeLocal(topic1, cb3), 0);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node.PublishLocal(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cb1Executed);
  cb1Executed = false;

  // Publish a second message on topic1
  EXPECT_EQ(node.PublishLocal(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cb1Executed);
  cb1Executed = false;

  // Unadvertise topic1 and publish a third message
  node.UnAdvertise(topic1);
  EXPECT_NE(node.PublishLocal(topic1, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cb1Executed);
}

//////////////////////////////////////////////////
/*TEST(DiscZmqTest, PubSubSameProcess)
{
  cb1Executed = false;
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  // Create the transport node
  transport::Node node(verbose);
  EXPECT_EQ(node.Advertise(topic1), 0);
  s_sleep(100);

  // Subscribe to topic1 in a different thread
  std::thread subscribeThread(CreateSubscriber);
  s_sleep(100);

  // Advertise and publish a msg on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  subscribeThread.join();

  // Check that the data was received
  EXPECT_TRUE(cb1Executed);
  cb1Executed = false;
}*/

//////////////////////////////////////////////////
/*TEST(DiscZmqTest, TwoSubscribersSameThread)
{
  cb1Executed = false;
  cb2Executed = false;
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  robot_msgs::StringMsg msg;
  msg.set_data(data);

  transport::Node node1(verbose);
  transport::Node node2(verbose);

  // Advertise topic1
  EXPECT_EQ(node1.Advertise(topic1), 0);

  // Subscribe to topic1 from node1
  EXPECT_EQ(node1.Subscribe(topic1, cb), 0);
  s_sleep(100);

  // Subscribe to topic1 from node2
  EXPECT_EQ(node2.Subscribe(topic1, cb2), 0);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node1.Publish(topic1, msg), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cb1Executed);
  cb1Executed = false;

  // Check that the msg was received
  EXPECT_TRUE(cb2Executed);
  cb2Executed = false;

  // Unadvertise topic1 and publish a third message
  node1.UnAdvertise(topic1);
  EXPECT_NE(node1.Publish(topic1, msg), 0);
  s_sleep(100);
  EXPECT_FALSE(cb1Executed);
  EXPECT_FALSE(cb2Executed);
}*/

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
