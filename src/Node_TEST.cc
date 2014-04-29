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
#include "ignition/transport/TransportTypes.hh"
#include "gtest/gtest.h"

using namespace ignition;

bool cbExecuted;
bool cbLocalExecuted;

static void s_sleep(int msecs)
{
  struct timespec t;
  t.tv_sec = msecs / 1000;
  t.tv_nsec = (msecs % 1000) * 1000000;
  nanosleep(&t, nullptr);
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cbLocal(const std::string &_topic, const transport::ProtoMsgPtr &_msgPtr)
{
  assert(_topic != "");

  robot_msgs::StringMsg *ptrMsg;
  ptrMsg = ::google::protobuf::down_cast<robot_msgs::StringMsg*>(_msgPtr.get());
  EXPECT_EQ(ptrMsg->data(), "someData");
  cbLocalExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic,
        const std::shared_ptr<robot_msgs::StringMsg> &_msgPtr)
{
  assert(_topic != "");

  EXPECT_EQ(_msgPtr->data(), "someData");
  cbExecuted = true;
}

//////////////////////////////////////////////////
void CreateSubscriber()
{
  std::string topic1 = "foo";
  bool verbose = false;
  transport::Node node(verbose);
  EXPECT_EQ(node.Subscribe(topic1, cb), 0);
  while (!cbExecuted)
    s_sleep(100);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubWithoutAdvertise)
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
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThread)
{
  cbExecuted = false;
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
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Publish a second message on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  // Unadvertise topic1 and publish a third message
  node.UnAdvertise(topic1);
  EXPECT_NE(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cbExecuted);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThreadLocal)
{
  cbLocalExecuted = false;
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());

  msgPtr->set_data(data);

  transport::Node node(verbose);

  // Advertise topic1
  EXPECT_EQ(node.Advertise(topic1), 0);

  // Subscribe to topic1
  node.SubscribeLocal(topic1, cbLocal);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the msg was received
  EXPECT_TRUE(cbLocalExecuted);
  cbLocalExecuted = false;

  // Publish a second message on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cbLocalExecuted);
  cbLocalExecuted = false;

  // Unadvertise topic1 and publish a third message
  node.UnAdvertise(topic1);
  EXPECT_NE(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);
  EXPECT_FALSE(cbLocalExecuted);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameProcess)
{
  cbExecuted = false;
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
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, SubscribeTemplated)
{
  cbExecuted = false;
  bool verbose = false;
  std::string topic1 = "foo";
  std::string data = "someData";
  std::shared_ptr<robot_msgs::StringMsg> msgPtr(new robot_msgs::StringMsg());
  msgPtr->set_data(data);

  // Create the transport node
  transport::Node node(verbose);
  EXPECT_EQ(node.Advertise(topic1), 0);
  s_sleep(100);

  EXPECT_EQ(node.Subscribe(topic1, cb), 0);
  s_sleep(100);

  // Publish a msg on topic1
  EXPECT_EQ(node.Publish(topic1, msgPtr), 0);
  s_sleep(100);

  // Check that the data was received
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;
}

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
