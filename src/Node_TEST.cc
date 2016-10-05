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
#include <csignal>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/test_config.h"
#include "msgs/int.pb.h"
#include "msgs/vector3d.pb.h"

using namespace ignition;

std::string partition;
std::string topic = "/foo";
std::mutex exitMutex;

int data = 5;
bool cbExecuted;
bool cb2Executed;
bool cbVectorExecuted;
bool srvExecuted;
bool responseExecuted;
bool wrongResponseExecuted;
int counter = 0;
bool terminatePub = false;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  cbExecuted = false;
  cb2Executed = false;
  srvExecuted = false;
  cbVectorExecuted = false;
  responseExecuted = false;
  wrongResponseExecuted = false;
  counter = 0;
  terminatePub = false;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const transport::msgs::Int &_msg)
{
  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
  counter++;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb2(const transport::msgs::Int &_msg)
{
  EXPECT_EQ(_msg.data(), data);
  cb2Executed = true;
}

//////////////////////////////////////////////////
/// \brief Provide a service call.
void srvEcho(const transport::msgs::Int &_req,
  transport::msgs::Int &_rep, bool &_result)
{
  srvExecuted = true;

  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Service call response callback.
void response(const transport::msgs::Int &_rep, const bool _result)
{
  EXPECT_EQ(_rep.data(), data);
  EXPECT_TRUE(_result);

  responseExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Service call response callback.
void wrongResponse(const transport::msgs::Vector3d &/*_rep*/, bool /*_result*/)
{
  wrongResponseExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Callback for receiving Vector3d data.
void cbVector(const transport::msgs::Vector3d &/*_msg*/)
{
  cbVectorExecuted = true;
}

//////////////////////////////////////////////////
/// \brief A class for testing subscription passing a member function
/// as a callback.
class MyTestClass
{
  /// \brief Class constructor.
  public: MyTestClass()
    : callbackExecuted(false),
      callbackSrvExecuted(false),
      wrongCallbackSrvExecuted(false)
  {
    // Subscribe to an illegal topic.
    EXPECT_FALSE(this->node.Subscribe("Bad Topic", &MyTestClass::Cb, this));

    EXPECT_TRUE(this->node.Subscribe(topic, &MyTestClass::Cb, this));
  }

  // Member function used as a callback for responding to a service call.
  public: void Echo(const transport::msgs::Int &_req,
    transport::msgs::Int &_rep, bool &_result)
  {
    EXPECT_EQ(_req.data(), data);
    _rep.set_data(_req.data());
    _result = true;
    this->callbackSrvExecuted = true;
  }

  /// \brief Member function used as a callback for responding to a service call
  public: void WrongEcho(const transport::msgs::Vector3d &/*_req*/,
    transport::msgs::Int &/*_rep*/, bool &_result)
  {
    _result = true;
    this->wrongCallbackSrvExecuted = true;
  }

  /// \brief Response callback to a service request.
  public: void EchoResponse(const transport::msgs::Int &_rep,
    const bool _result)
  {
    EXPECT_EQ(_rep.data(), data);
    EXPECT_TRUE(_result);

    this->responseExecuted = true;
  }

  /// \brief Member function called each time a topic update is received.
  public: void Cb(const transport::msgs::Int &_msg)
  {
    EXPECT_EQ(_msg.data(), data);
    this->callbackExecuted = true;
  };

  /// \brief Advertise a topic and publish a message.
  public: void SendSomeData()
  {
    transport::msgs::Int msg;
    msg.set_data(data);

    // Advertise an illegal topic.
    EXPECT_FALSE(this->node.Advertise<transport::msgs::Int>("invalid topic"));

    EXPECT_TRUE(this->node.Advertise<transport::msgs::Int>(topic));
    EXPECT_TRUE(this->node.Publish(topic, msg));
  }

  public: void TestServiceCall()
  {
    transport::msgs::Int req;
    transport::msgs::Int rep;
    transport::msgs::Vector3d wrongReq;
    transport::msgs::Vector3d wrongRep;
    int timeout = 500;
    bool result;

    req.set_data(data);

    this->Reset();

    // Advertise an illegal service name.
    EXPECT_FALSE(this->node.Advertise("Bad Srv", &MyTestClass::Echo, this));

    // Advertise and request a valid service.
    EXPECT_TRUE(this->node.Advertise(topic, &MyTestClass::Echo, this));
    EXPECT_TRUE(this->node.Request(topic, req, timeout, rep, result));
    ASSERT_TRUE(result);
    EXPECT_EQ(rep.data(), data);
    EXPECT_TRUE(this->callbackSrvExecuted);

    this->Reset();

    // Request a valid service using a member function callback.
    this->node.Request(topic, req, &MyTestClass::EchoResponse, this);
    EXPECT_TRUE(this->responseExecuted);

    this->Reset();

    // Service requests with wrong types.
    EXPECT_FALSE(this->node.Request(topic, wrongReq, timeout, rep, result));
    EXPECT_FALSE(this->node.Request(topic, req, timeout, wrongRep, result));
    EXPECT_TRUE(this->node.Request(topic, wrongReq, response));
    EXPECT_TRUE(this->node.Request(topic, req, wrongResponse));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(this->callbackSrvExecuted);
    EXPECT_FALSE(this->wrongCallbackSrvExecuted);
  }

  public: void Reset()
  {
    this->callbackExecuted = false;
    this->callbackSrvExecuted = false;
    this->wrongCallbackSrvExecuted = false;
    this->responseExecuted = false;
  }

  /// \brief Member variables that flag when the callbacks are executed.
  public: bool callbackExecuted;
  public: bool callbackSrvExecuted;
  public: bool wrongCallbackSrvExecuted;
  public: bool responseExecuted;

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
void CreatePubSubTwoThreads(
  const transport::Scope_t &_sc = transport::Scope_t::ALL)
{
  reset();

  transport::AdvertiseOptions opts;
  opts.SetScope(_sc);

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic, opts));

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

  transport::msgs::Int msg;
  msg.set_data(data);

  // Check that an invalid namespace is ignored. The callbacks are expecting an
  // empty namespace.
  transport::NodeOptions optionsNode1;
  transport::NodeOptions optionsNode2;
  optionsNode1.SetPartition(partition);
  optionsNode1.SetNameSpace("invalid namespace");
  optionsNode2.SetPartition(partition);
  transport::Node node1(optionsNode1);
  transport::Node node2(optionsNode2);

  // Check the advertised/subscribed topics and advertised services.
  EXPECT_TRUE(node1.AdvertisedTopics().empty());
  EXPECT_TRUE(node1.SubscribedTopics().empty());
  EXPECT_TRUE(node1.AdvertisedServices().empty());

  // Publish some data on topic without advertising it first.
  EXPECT_FALSE(node1.Publish(topic, msg));

  EXPECT_TRUE(node1.Advertise<transport::msgs::Int>(topic));

  auto advertisedTopics = node1.AdvertisedTopics();
  ASSERT_EQ(advertisedTopics.size(), 1u);
  EXPECT_EQ(advertisedTopics.at(0), topic);

  EXPECT_TRUE(node2.Advertise<transport::msgs::Int>(topic));
  advertisedTopics = node2.AdvertisedTopics();
  ASSERT_EQ(advertisedTopics.size(), 1u);
  EXPECT_EQ(advertisedTopics.at(0), topic);

  EXPECT_TRUE(node2.Subscribe(topic, cb));
  auto subscribedTopics = node2.SubscribedTopics();
  ASSERT_EQ(subscribedTopics.size(), 1u);
  EXPECT_EQ(subscribedTopics.at(0), topic);

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

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node;

  // Advertise an illegal topic.
  EXPECT_FALSE(node.Advertise<transport::msgs::Int>("invalid topic"));

  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic));

  // Subscribe to an illegal topic.
  EXPECT_FALSE(node.Subscribe("invalid topic", cb));

  EXPECT_TRUE(node.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Try to publish a message using an invalid topic.
  EXPECT_FALSE(node.Publish("invalid topic", msg));

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

  reset();

  // Unadvertise an illegal topic.
  EXPECT_FALSE(node.Unadvertise("invalid topic"));

  EXPECT_TRUE(node.Unadvertise(topic));

  // Publish a third message.
  EXPECT_FALSE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(cbExecuted);
}

//////////////////////////////////////////////////
/// \brief Subscribe to a topic using a lambda function.
TEST(NodeTest, PubSubSameThreadLamda)
{
  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic));

  bool executed = false;
  std::function<void(const transport::msgs::Int&)> subCb =
    [&executed](const transport::msgs::Int &_msg)
  {
    EXPECT_EQ(_msg.data(), data);
    executed = true;
  };

  EXPECT_TRUE(node.Subscribe(topic, subCb));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a first message.
  EXPECT_TRUE(node.Publish(topic, msg));

  EXPECT_TRUE(executed);
}

//////////////////////////////////////////////////
/// \brief Advertise two topics with the same name. It's not possible to do it
/// within the same node but it's valid on separate nodes.
TEST(NodeTest, AdvertiseTwoEqualTopics)
{
  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise<transport::msgs::Int>(topic));
  EXPECT_FALSE(node1.Advertise<transport::msgs::Vector3d>(topic));
  EXPECT_TRUE(node2.Advertise<transport::msgs::Vector3d>(topic));
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

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise<transport::msgs::Int>(topic));

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

  auto subscribedTopics = node1.SubscribedTopics();
  ASSERT_EQ(subscribedTopics.size(), 1u);
  EXPECT_EQ(subscribedTopics.at(0), topic);

  reset();

  // Try to unsubscribe from an invalid topic.
  EXPECT_FALSE(node1.Unsubscribe("invalid topic"));

  // Node1 is not interested in the topic anymore.
  EXPECT_TRUE(node1.Unsubscribe(topic));

  // Give some time to receive the unsubscription.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Publish a second message.
  EXPECT_TRUE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was not received by node1.
  EXPECT_FALSE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  ASSERT_TRUE(node1.SubscribedTopics().empty());

  reset();

  EXPECT_TRUE(node1.Unadvertise(topic));

  // Publish a third message
  EXPECT_FALSE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Anybody should have received the message.
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(cb2Executed);

  auto subscribedServices = node1.AdvertisedServices();
  ASSERT_TRUE(subscribedServices.empty());
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

  client.TestServiceCall();
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in different threads are able to communicate
/// advertising a topic with "Process" scope.
TEST(NodeTest, ScopeProcess)
{
  CreatePubSubTwoThreads(transport::Scope_t::PROCESS);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in different threads are able to communicate
/// advertising a topic with "Host" scope.
TEST(NodeTest, ScopeHost)
{
  CreatePubSubTwoThreads(transport::Scope_t::HOST);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in different threads are able to communicate
/// advertising a topic with "All" scope.
TEST(NodeTest, ScopeAll)
{
  CreatePubSubTwoThreads(transport::Scope_t::ALL);
}

//////////////////////////////////////////////////
/// \brief Check that the types advertised and published match.
TEST(NodeTest, TypeMismatch)
{
  transport::msgs::Int rightMsg;
  transport::msgs::Vector3d wrongMsg;
  rightMsg.set_data(1);
  wrongMsg.set_x(1);
  wrongMsg.set_y(2);
  wrongMsg.set_z(3);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic));

  EXPECT_FALSE(node.Publish(topic, wrongMsg));
  EXPECT_TRUE(node.Publish(topic, rightMsg));
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallAsync)
{
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  transport::msgs::Int req;
  req.set_data(data);

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvEcho));

  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  auto advertisedServices = node.AdvertisedServices();
  ASSERT_EQ(advertisedServices.size(), 1u);
  EXPECT_EQ(advertisedServices.at(0), topic);

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, response));

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

  // Try to unadvertise an invalid service.
  EXPECT_FALSE(node.UnadvertiseSrv("invalid service"));

  EXPECT_TRUE(node.UnadvertiseSrv(topic));

  ASSERT_TRUE(node.AdvertisedServices().empty());
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous service call using lambdas.
TEST(NodeTest, ServiceCallAsyncLambda)
{
  std::function<void(const transport::msgs::Int &, transport::msgs::Int &,
    bool &)> advCb = [](const transport::msgs::Int &_req,
      transport::msgs::Int &_rep, bool &_result)
  {
    EXPECT_EQ(_req.data(), data);
    _rep.set_data(_req.data());
    _result = true;
  };

  transport::Node node;
  EXPECT_TRUE((node.Advertise<transport::msgs::Int, transport::msgs::Int>(topic,
    advCb)));

  bool executed = false;
  std::function<void(const transport::msgs::Int &, const bool)> reqCb =
    [&executed](const transport::msgs::Int &_rep, const bool _result)
  {
    EXPECT_EQ(_rep.data(), data);
    EXPECT_TRUE(_result);
    executed = true;
  };

  transport::msgs::Int req;
  req.set_data(data);

  EXPECT_TRUE((node.Request(topic, req, reqCb)));

  EXPECT_TRUE(executed);
}

//////////////////////////////////////////////////
/// \brief Request multiple service calls at the same time.
TEST(NodeTest, MultipleServiceCallAsync)
{
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  transport::msgs::Int req;
  req.set_data(data);

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvEcho));

  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, response));

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
  EXPECT_TRUE(node.Request(topic, req, response));
  EXPECT_TRUE(node.Request(topic, req, response));

  i = 0;
  while (i < 100 && counter < 3)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 3);

  // Try to unadvertise an invalid service.
  EXPECT_FALSE(node.UnadvertiseSrv("invalid service"));

  EXPECT_TRUE(node.UnadvertiseSrv(topic));
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallSync)
{
  transport::msgs::Int req;
  transport::msgs::Int rep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, timeout, rep, result));

  EXPECT_TRUE(node.Request(topic, req, timeout, rep, result));

  // Check that the service call response was executed.
  EXPECT_TRUE(result);
  EXPECT_EQ(rep.data(), req.data());
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallSyncTimeout)
{
  transport::msgs::Int req;
  transport::msgs::Int rep;
  bool result;
  int64_t timeout = 1000;

  req.set_data(data);

  transport::Node node;

  auto t1 = std::chrono::system_clock::now();
  bool executed = node.Request(topic, req, static_cast<unsigned int>(timeout),
      rep, result);
  auto t2 = std::chrono::system_clock::now();

  int64_t elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was close to the timeout.
  auto diff = std::max(elapsed, timeout) - std::min(elapsed, timeout);
  EXPECT_LE(diff, 10);

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
  transport::msgs::Int msg;
  msg.set_data(data);
  transport::Node node;

  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic));

  auto i = 0;
  bool exitLoop = false;
  while (!exitLoop)
  {
    EXPECT_TRUE(node.Publish(topic, msg));
    ++i;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    {
      std::lock_guard<std::mutex> lock(exitMutex);
      if (terminatePub)
        exitLoop = true;
    }
  }

  EXPECT_LT(i, 200);
}

//////////////////////////////////////////////////
/// \brief Capture SIGINT and SIGTERM and flag that we want to exit.
void signal_handler(int _signal)
{
  std::lock_guard<std::mutex> lock(exitMutex);
  if (_signal == SIGINT || _signal == SIGTERM)
    terminatePub = true;
}

//////////////////////////////////////////////////
/// \brief Check that an external program can capture a SIGINT and terminate
/// the program without problems.
TEST(NodeTest, SigIntTermination)
{
  reset();

  // Install a signal handler for SIGINT.
  std::signal(SIGINT, signal_handler);

  auto thread = std::thread(createInfinitePublisher);
#ifdef _WIN32
  thread.detach();
#endif

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::raise(SIGINT);

#ifndef _WIN32
  if (thread.joinable())
    thread.join();
#else
  WaitForSingleObject(thread.native_handle(), INFINITE);
  CloseHandle(thread.native_handle());
#endif
}

//////////////////////////////////////////////////
/// \brief Check that an external program can capture a SIGTERM and terminate
/// the program without problems.
TEST(NodeTest, SigTermTermination)
{
  reset();

  // Install a signal handler for SIGTERM.
  std::signal(SIGTERM, signal_handler);

  auto thread = std::thread(createInfinitePublisher);
#ifdef _WIN32
  thread.detach();
#endif

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::raise(SIGTERM);

#ifndef _WIN32
  if (thread.joinable())
    thread.join();
#else
  WaitForSingleObject(thread.native_handle(), INFINITE);
  CloseHandle(thread.native_handle());
#endif
}

//////////////////////////////////////////////////
/// \brief Check that a message is not published if the type does not match
/// the type advertised.
TEST(NodeTest, PubSubWrongTypesOnPublish)
{
  reset();

  transport::msgs::Int msg;
  msg.set_data(data);
  transport::msgs::Vector3d msgV;
  msgV.set_x(1);
  msgV.set_y(2);
  msgV.set_z(3);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<transport::msgs::Int>(topic));

  EXPECT_TRUE(node.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send a message with a wrong type.
  EXPECT_FALSE(node.Publish(topic, msgV));

  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);

  reset();

  // Publish a second message on topic.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the data was received.
  EXPECT_TRUE(cbExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief Check that a message is not received if the callback does not use
/// the advertised types.
TEST(NodeTest, PubSubWrongTypesOnSubscription)
{
  reset();

  transport::msgs::Vector3d msgV;
  msgV.set_x(1);
  msgV.set_y(2);
  msgV.set_z(3);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<transport::msgs::Vector3d>(topic));

  EXPECT_TRUE(node.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send a message with a wrong type.
  EXPECT_TRUE(node.Publish(topic, msgV));

  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns two subscribers on the same topic. One of the
/// subscribers has a wrong callback (types in the callback does not match the
/// advertised type). Check that only the good callback is executed.
TEST(NodeTest, PubSubWrongTypesTwoSubscribers)
{
  reset();

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise<transport::msgs::Int>(topic));

  // Good subscriber.
  EXPECT_TRUE(node1.Subscribe(topic, cb));

  // Bad subscriber: cbVector does not match the types advertised by node1.
  EXPECT_TRUE(node2.Subscribe(topic, cbVector));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the message was received by node1.
  EXPECT_TRUE(cbExecuted);

  // Check that the message was not received by node2.
  EXPECT_FALSE(cbVectorExecuted);
}

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and a service requester. The
/// requester uses a wrong type for the request argument. The test should verify
/// that the service call does not succeed.
TEST(NodeTest, SrvRequestWrongReq)
{
  transport::msgs::Vector3d wrongReq;
  transport::msgs::Int rep;
  bool result;
  unsigned int timeout = 1000;

  wrongReq.set_x(1);
  wrongReq.set_y(2);
  wrongReq.set_z(3);

  reset();

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request an asynchronous service call with wrong type in the request.
  EXPECT_TRUE(node.Request(topic, wrongReq, response));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(responseExecuted);

  // Request a synchronous service call with wrong type in the request.
  EXPECT_FALSE(node.Request(topic, wrongReq, timeout, rep, result));

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and a service requester. The
/// requester uses a wrong type for the response argument. The test should
/// verify that the service call does not succeed.
TEST(NodeTest, SrvRequestWrongRep)
{
  transport::msgs::Int req;
  transport::msgs::Vector3d wrongRep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  reset();

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request an asynchronous service call with wrong type in the response.
  EXPECT_TRUE(node.Request(topic, req, wrongResponse));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(wrongResponseExecuted);

  // Request a synchronous service call with wrong type in the response.
  EXPECT_FALSE(node.Request(topic, req, timeout, wrongRep, result));

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and two service requesters. One
/// requester uses wrong type arguments. The test should verify that only one
/// of the requesters receives the response.
TEST(NodeTest, SrvTwoRequestsOneWrong)
{
  transport::msgs::Int req;
  transport::msgs::Int goodRep;
  transport::msgs::Vector3d badRep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request service calls with wrong types in the response.
  EXPECT_FALSE(node.Request(topic, req, timeout, badRep, result));
  EXPECT_TRUE(node.Request(topic, req, wrongResponse));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(wrongResponseExecuted);

  // Valid service requests.
  EXPECT_TRUE(node.Request(topic, req, timeout, goodRep, result));
  EXPECT_TRUE(node.Request(topic, req, response));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_TRUE(responseExecuted);
}

//////////////////////////////////////////////////
/// \brief This test creates two nodes and advertises some topics. The test
/// verifies that TopicList() returns the list of all the topics advertised.
TEST(NodeTest, TopicList)
{
  std::vector<std::string> topics;
  transport::Node node1;
  transport::Node node2;

  node1.Advertise<transport::msgs::Int>("topic1");
  node2.Advertise<transport::msgs::Int>("topic2");

  node1.TopicList(topics);
  EXPECT_EQ(topics.size(), 2u);
  topics.clear();

  auto start = std::chrono::steady_clock::now();
  node1.TopicList(topics);
  auto end = std::chrono::steady_clock::now();
  EXPECT_EQ(topics.size(), 2u);

  // The first TopicList() call might block if the discovery is still
  // initializing (it may happen if we run this test alone).
  // However, the second call should never block.
  auto elapsed = end - start;
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>
      (elapsed).count(), 2);
}

//////////////////////////////////////////////////
/// \brief This test creates two nodes and advertises some services. The test
/// verifies that ServiceList() returns the list of all the services advertised.
TEST(NodeTest, ServiceList)
{
  std::vector<std::string> services;
  transport::Node node;

  node.Advertise(topic, srvEcho);

  node.ServiceList(services);
  EXPECT_EQ(services.size(), 1u);
  services.clear();

  auto start = std::chrono::steady_clock::now();
  node.ServiceList(services);
  auto end = std::chrono::steady_clock::now();
  EXPECT_EQ(services.size(), 1u);

  // The first TopicList() call might block if the discovery is still
  // initializing (it may happen if we run this test alone).
  //  However, the second call should never block.
  auto elapsed = end - start;
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>
      (elapsed).count(), 2);
}

//////////////////////////////////////////////////
/// \brief Create a separate thread, block it calling waitForShutdown() and
/// emit a SIGINT signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(NodeTest, waitForShutdownSIGINT)
{
  std::thread aThread([]{ignition::transport::waitForShutdown();});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  aThread.join();
}

//////////////////////////////////////////////////
/// \brief Create a separate thread, block it calling waitForShutdown() and
/// emit a SIGTERM signal. Check that the transport library captures the signal
/// and is able to terminate.TEST(NodeTest, TerminateSIGTERM)
TEST(NodeTest, waitForShutdownSIGTERM)
{
  std::thread aThread([]{ignition::transport::waitForShutdown();});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGTERM);
  aThread.join();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
