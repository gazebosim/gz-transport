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
#include <chrono>
#include <csignal>
#include <string>
#include <thread>
#include "gtest/gtest.h"
#include "ignition/transport/Node.hh"

using namespace ignition;

std::string topic = "/foo";
std::string data = "bar";
bool cbExecuted;
bool cb2Executed;
bool srvExecuted;
bool responseExecuted;
int counter = 0;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  cbExecuted = false;
  cb2Executed = false;
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const std::string &_topic, const robot_msgs::StringMsg &_msg)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
  counter++;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb2(const std::string &_topic, const robot_msgs::StringMsg &_msg)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_msg.data(), data);
  cb2Executed = true;
}

//////////////////////////////////////////////////
/// \brief Provide a service call.
void srvEcho(const std::string &_topic, const robot_msgs::StringMsg &_req,
  robot_msgs::StringMsg &_rep, bool &_result)
{
  EXPECT_EQ(_topic, topic);
  srvExecuted = true;

  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());
  _result = true;
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
/// \brief A class for testing subscription passing a member function
/// as a callback.
class MyTestClass
{
  /// \brief Class constructor.
  public: MyTestClass()
    : callbackExecuted(false)
  {
    EXPECT_TRUE(this->node.Subscribe(topic, &MyTestClass::Cb, this));
  }

  /// \brief Member function called each time a topic update is received.
  public: void Cb(const std::string &_topic, const robot_msgs::StringMsg &_msg)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_msg.data(), data);
    this->callbackExecuted = true;
  };

  /// \brief Advertise a topic and publish a message.
  public: void SendSomeData()
  {
    robot_msgs::StringMsg msg;
    msg.set_data(data);

    EXPECT_TRUE(this->node.Advertise(topic));
    EXPECT_TRUE(this->node.Publish(topic, msg));
  }

  /// \brief Member variable that flags when the callback is executed.
  public: bool callbackExecuted;

  /// \brief Transport node;
  private: transport::Node node;
};

//////////////////////////////////////////////////
/// \brief Create a subscriber and wait for a callback to be executed.
void CreateSubscriber()
{
  transport::Node node;
  EXPECT_TRUE(node.Subscribe(topic, cb));

  int i = 0;
  while (i < 100 && !cbExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }
}

//////////////////////////////////////////////////
/// \brief Use two threads using their own transport nodes. One thread
/// will publish a message, whereas the other thread is subscribed to the topic.
/// \param[in] _scope Scope used to advertise the topic.
void CreatePubSubTwoThreads(const transport::Scope &_sc = transport::Scope::All)
{
  reset();

  robot_msgs::StringMsg msg;
  msg.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, _sc));

  // Subscribe to a topic in a different thread and wait until the callback is
  // received.
  std::thread subscribeThread(CreateSubscriber);

  // Wait some time until the subscriber is alive.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a msg on topic.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Wait until the subscribe thread finishes.
  subscribeThread.join();

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);
}

//////////////////////////////////////////////////
/// \brief A message should not be published if it is not advertised before.
TEST(NodeTest, PubWithoutAdvertise)
{
  reset();

  robot_msgs::StringMsg msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  // Publish some data on topic without advertising it first.
  EXPECT_FALSE(node1.Publish(topic, msg));

  EXPECT_TRUE(node1.Advertise(topic));
  EXPECT_TRUE(node2.Advertise(topic));

  EXPECT_TRUE(node2.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a message by each node.
  EXPECT_TRUE(node1.Publish(topic, msg));
  EXPECT_TRUE(node2.Publish(topic, msg));

  // Wait some time for the messages to arrive.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was received twice.
  EXPECT_TRUE(cbExecuted);
  EXPECT_EQ(counter, 2);
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, PubSubSameThread)
{
  reset();

  robot_msgs::StringMsg msg;
  msg.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic));

  EXPECT_TRUE(node.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a first message.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);

  reset();

  // Publish a second message on topic.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the data was received.
  EXPECT_TRUE(cbExecuted);
  cbExecuted = false;

  EXPECT_TRUE(node.Unadvertise(topic));

  // Publish a third message.
  EXPECT_FALSE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(cbExecuted);
}

//////////////////////////////////////////////////
/// \brief Use two threads using their own transport nodes. One thread
/// will publish a message, whereas the other thread is subscribed to the topic.
TEST(NodeTest, PubSubTwoThreadsSameTopic)
{
  CreatePubSubTwoThreads();
}

//////////////////////////////////////////////////
/// \brief Use two different transport node on the same thread. Check that
/// both receive the updates when they are subscribed to the same topic. Check
/// also that when one of the nodes unsubscribes, no longer receives updates.
TEST(NodeTest, PubSubOneThreadTwoSubs)
{
  reset();

  robot_msgs::StringMsg msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise(topic));

  // Subscribe to topic in node1.
  EXPECT_TRUE(node1.Subscribe(topic, cb));

  // Subscribe to topic in node2.
  EXPECT_TRUE(node2.Subscribe(topic, cb2));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was received by node1.
  EXPECT_TRUE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  reset();

  // Node1 is not interested in the topic anymore.
  EXPECT_TRUE(node1.Unsubscribe(topic));

  // Give some time to receive the unsubscription.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Publish a second message.
  EXPECT_TRUE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was no received by node1.
  EXPECT_FALSE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  reset();

  EXPECT_TRUE(node1.Unadvertise(topic));

  // Publish a third message
  EXPECT_FALSE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Anybody should have received the message.
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(cb2Executed);
}

//////////////////////////////////////////////////
/// \brief Use the transport inside a class and check advertise, subscribe and
/// publish.
TEST(NodeTest, ClassMemberCallback)
{
  MyTestClass client;

  // Wait for the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  client.SendSomeData();

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(client.callbackExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in different threads are able to communicate
/// advertising a topic with "Process" scope.
TEST(NodeTest, ScopeProcess)
{
  CreatePubSubTwoThreads(transport::Scope::Process);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in diffetent threads are able to communicate
/// advertising a topic with "Host" scope.
TEST(NodeTest, ScopeHost)
{
  CreatePubSubTwoThreads(transport::Scope::Host);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in diffetent threads are able to communicate
/// advertising a topic with "All" scope.
TEST(NodeTest, ScopeAll)
{
  CreatePubSubTwoThreads(transport::Scope::All);
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallAsync)
{
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  robot_msgs::StringMsg req;
  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));
  EXPECT_TRUE(node.Request(topic, req, response));

  int i = 0;
  while (i < 100 && !srvExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);

  // Make another request.
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  EXPECT_TRUE(node.Request(topic, req, response));

  i = 0;
  while (i < 100 && !responseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallSync)
{
  robot_msgs::StringMsg req;
  robot_msgs::StringMsg rep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));
  bool executed = node.Request(topic, req, timeout, rep, result);

  // Check that the service call response was executed.
  EXPECT_TRUE(executed);
  EXPECT_EQ(rep.data(), req.data());
  EXPECT_TRUE(result);
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallSyncTimeout)
{
  robot_msgs::StringMsg req;
  robot_msgs::StringMsg rep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;

  auto t1 = std::chrono::system_clock::now();
  bool executed = node.Request(topic, req, timeout, rep, result);
  auto t2 = std::chrono::system_clock::now();

  double elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was close to the timeout.
  EXPECT_NEAR(elapsed, timeout, 1.0);

  // Check that the service call response was not executed.
  EXPECT_FALSE(executed);
}

//////////////////////////////////////////////////
/// \brief Create a publisher that sends messages "forever". This function will
/// be used emiting a SIGINT or SIGTERM signal, to make sure that the transport
/// library captures the signals, stop all the tasks and signal the event with
/// the method Interrupted().
void createInfinitePublisher()
{
  robot_msgs::StringMsg msg;
  msg.set_data(data);
  transport::Node node;

  EXPECT_TRUE(node.Advertise(topic));

  auto i = 0;
  while (i < 200 && !node.Interrupted())
  {
    EXPECT_TRUE(node.Publish(topic, msg));
    ++i;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_LT(i, 200);
}

//////////////////////////////////////////////////
/// \brief Create a transport client in a loop (and in a separate thread) and
/// emit a SIGINT signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(NodeTest, TerminateSIGINT)
{
  std::thread publisherThread(createInfinitePublisher);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  publisherThread.join();
}

//////////////////////////////////////////////////
/// \brief Create a transport client in a loop (and in a separate thread) and
/// emit a SIGTERM signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(NodeTest, TerminateSIGTERM)
{
  std::thread publisherThread(createInfinitePublisher);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  raise(SIGTERM);
  publisherThread.join();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
