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
#include <ignition/msgs.hh>

#include "gtest/gtest.h"
#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

static std::string partition;
static std::string g_topic = "/foo";
static std::mutex exitMutex;

static int data = 5;
static bool cbExecuted;
static bool cb2Executed;
static bool cbVectorExecuted;
static bool srvExecuted;
static bool responseExecuted;
static bool wrongResponseExecuted;
static int counter = 0;
static bool terminatePub = false;

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
void cb(const ignition::msgs::Int32 &_msg)
{
  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
  counter++;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb2(const ignition::msgs::Int32 &_msg)
{
  EXPECT_EQ(_msg.data(), data);
  cb2Executed = true;
}

//////////////////////////////////////////////////
/// \brief Provide a service call.
void srvEcho(const ignition::msgs::Int32 &_req,
  ignition::msgs::Int32 &_rep, bool &_result)
{
  srvExecuted = true;

  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Provide a service call without input.
void srvWithoutInput(ignition::msgs::Int32 &_rep, bool &_result)
{
  srvExecuted = true;
  _rep.set_data(data);
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Provide a service call without waiting for response.
void srvWithoutOutput(const ignition::msgs::Int32 &_req)
{
  srvExecuted = true;

  EXPECT_EQ(_req.data(), data);
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Service call response callback.
void response(const ignition::msgs::Int32 &_rep, const bool _result)
{
  EXPECT_EQ(_rep.data(), data);
  EXPECT_TRUE(_result);

  responseExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Service call response callback.
void wrongResponse(const ignition::msgs::Vector3d &/*_rep*/, bool /*_result*/)
{
  wrongResponseExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Callback for receiving Vector3d data.
void cbVector(const ignition::msgs::Vector3d &/*_msg*/)
{
  cbVectorExecuted = true;
}

//////////////////////////////////////////////////
/// \brief A class for testing subscription, advertisement, and request passing
/// a member function as a callback.

class MyTestClass
{
  /// \brief Class constructor.
  public: MyTestClass()
    : callbackExecuted(false),
      callbackSrvExecuted(false),
      responseExecuted(false)
  {
  }

  /// \brief Create a subscriber.
  public: void Subscribe()
  {
    // Subscribe to an illegal topic.
    EXPECT_FALSE(this->node.Subscribe("Bad Topic", &MyTestClass::Cb, this));

    EXPECT_TRUE(this->node.Subscribe(g_topic, &MyTestClass::Cb, this));
  }

  /// \brief Member function used as a callback for responding to a service
  /// call.
  public: void Echo(const ignition::msgs::Int32 &_req,
    ignition::msgs::Int32 &_rep, bool &_result)
  {
    EXPECT_EQ(_req.data(), data);
    _rep.set_data(_req.data());
    _result = true;
    this->callbackSrvExecuted = true;
  }

  /// \brief Member function used as a callback for responding to a service
  /// call without input.
  public: void WithoutInput(ignition::msgs::Int32 &_rep, bool &_result)
  {
    _rep.set_data(data);
    _result = true;
    this->callbackSrvExecuted = true;
  }

  // Member function used as a callback for responding to a service call
  // without without waiting for a response.
  public: void WithoutOutput(const ignition::msgs::Int32 &_req)
  {
    EXPECT_EQ(_req.data(), data);
    this->callbackSrvExecuted = true;
  }

  /// \brief Response callback to a service request.
  public: void EchoResponse(const ignition::msgs::Int32 &_rep,
    const bool _result)
  {
    EXPECT_EQ(_rep.data(), data);
    EXPECT_TRUE(_result);

    this->responseExecuted = true;
  }

  /// \brief Response callback to a service request without input.
  public: void WithoutInputResponse(const ignition::msgs::Int32 &_rep,
    const bool _result)
  {
    EXPECT_EQ(_rep.data(), data);
    EXPECT_TRUE(_result);

    this->responseExecuted = true;
  }

  /// \brief Member function called each time a topic update is received.
  public: void Cb(const ignition::msgs::Int32 &_msg)
  {
    EXPECT_EQ(_msg.data(), data);
    this->callbackExecuted = true;
  };

  /// \brief Advertise a topic and publish a message.
  public: void SendSomeData()
  {
    ignition::msgs::Int32 msg;
    msg.set_data(data);

    // Advertise an illegal topic.
    EXPECT_FALSE(this->node.Advertise<ignition::msgs::Int32>("invalid topic"));

    auto pubId = this->node.Advertise<ignition::msgs::Int32>("invalid topic");
    EXPECT_FALSE(pubId);
    EXPECT_FALSE(pubId.Valid());
    EXPECT_TRUE(pubId.Topic().empty());

    pubId = this->node.Advertise<ignition::msgs::Int32>(g_topic);
    EXPECT_TRUE(pubId);
    EXPECT_TRUE(pubId.Valid());
    EXPECT_FALSE(pubId.Topic().empty());
    EXPECT_TRUE(pubId.Topic().find(g_topic) != std::string::npos);
    EXPECT_TRUE(this->node.Publish(pubId, msg));
  }

  /// \brief Advertise a service, request a service using non-blocking and
  /// blocking call.
  public: void TestServiceCall()
  {
    ignition::msgs::Int32 req;
    ignition::msgs::Int32 rep;
    ignition::msgs::Vector3d wrongReq;
    ignition::msgs::Vector3d wrongRep;
    int timeout = 500;
    bool result;

    req.set_data(data);

    this->Reset();

    // Advertise an illegal service name.
    EXPECT_FALSE(this->node.Advertise("Bad Srv", &MyTestClass::Echo, this));

    // Advertise and request a valid service.
    EXPECT_TRUE(this->node.Advertise(g_topic, &MyTestClass::Echo, this));
    EXPECT_TRUE(this->node.Request(g_topic, req, timeout, rep, result));
    ASSERT_TRUE(result);
    EXPECT_EQ(rep.data(), data);
    EXPECT_TRUE(this->callbackSrvExecuted);

    this->Reset();

    // Request a valid service using a member function callback.
    this->node.Request(g_topic, req, &MyTestClass::EchoResponse, this);
    EXPECT_TRUE(this->responseExecuted);

    this->Reset();

    // Service requests with wrong types.
    EXPECT_FALSE(this->node.Request(g_topic, wrongReq, timeout, rep, result));
    EXPECT_FALSE(this->node.Request(g_topic, req, timeout, wrongRep, result));
    EXPECT_TRUE(this->node.Request(g_topic, wrongReq, response));
    EXPECT_TRUE(this->node.Request(g_topic, req, wrongResponse));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(this->callbackSrvExecuted);
  }

  /// \brief Advertise a service without input, request a service without input
  /// using non-blocking and blocking call.
  public: void TestServiceCallWithoutInput()
  {
    ignition::msgs::Int32 rep;
    ignition::msgs::Vector3d wrongRep;
    int timeout = 500;
    bool result;

    this->Reset();

    // Advertise an illegal service name without input.
    EXPECT_FALSE(this->node.Advertise("Bad Srv", &MyTestClass::WithoutInput,
      this));

    // Advertise and request a valid service without input.
    EXPECT_TRUE(this->node.Advertise(g_topic, &MyTestClass::WithoutInput,
      this));
    EXPECT_TRUE(this->node.Request(g_topic, timeout, rep, result));
    ASSERT_TRUE(result);
    EXPECT_EQ(rep.data(), data);
    EXPECT_TRUE(this->callbackSrvExecuted);

    this->Reset();

    // Request a valid service without input using a member function callback.
    this->node.Request(g_topic, &MyTestClass::WithoutInputResponse, this);
    EXPECT_TRUE(this->responseExecuted);

    this->Reset();

    // Service requests without input with wrong types.
    EXPECT_FALSE(this->node.Request(g_topic, timeout, wrongRep, result));
    EXPECT_TRUE(this->node.Request(g_topic, wrongResponse));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(this->callbackSrvExecuted);
  }

  /// \brief Advertise and request a service without waiting for response.
  public: void TestServiceCallWithoutOutput()
  {
    ignition::msgs::Int32 req;
    ignition::msgs::Vector3d wrongReq;

    req.set_data(data);

    this->Reset();

    // Advertise an illegal service name without waiting for response.
    EXPECT_FALSE(this->node.Advertise("Bad Srv", &MyTestClass::WithoutOutput,
                                      this));

    // Advertise and request a valid service without waiting for response.
    EXPECT_TRUE(this->node.Advertise(g_topic, &MyTestClass::WithoutOutput,
                                     this));
    EXPECT_TRUE(this->node.Request(g_topic, req));
    EXPECT_TRUE(this->callbackSrvExecuted);

    this->Reset();

    // Service requests without waiting for response with wrong types.
    EXPECT_TRUE(this->node.Request(g_topic, wrongReq));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(this->callbackSrvExecuted);
  }

  public: void Reset()
  {
    this->callbackExecuted = false;
    this->callbackSrvExecuted = false;
    this->responseExecuted = false;
  }

  /// \brief Member variables that flag when the actions are executed.
  public: bool callbackExecuted;
  public: bool callbackSrvExecuted;
  public: bool responseExecuted;

  /// \brief Transport node;
  private: transport::Node node;
};

//////////////////////////////////////////////////
/// \brief Create a subscriber and wait for a callback to be executed.
void CreateSubscriber()
{
  transport::Node node;
  EXPECT_TRUE(node.Subscribe(g_topic, cb));

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

  ignition::msgs::Int32 msg;
  msg.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise<ignition::msgs::Int32>(g_topic, opts));

  // Subscribe to a topic in a different thread and wait until the callback is
  // received.
  std::thread subscribeThread(CreateSubscriber);

  // Wait some time until the subscriber is alive.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a msg on topic.
  EXPECT_TRUE(node.Publish(g_topic, msg));

  // Wait until the subscribe thread finishes.
  subscribeThread.join();

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief A message should not be published if it is not advertised before.
TEST(NodeTest, PubWithoutAdvertise)
{
  reset();

  ignition::msgs::Int32 msg;
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
  EXPECT_FALSE(node1.Publish(g_topic, msg));

  EXPECT_TRUE(node1.Advertise(g_topic, msg.GetTypeName()));

  auto advertisedTopics = node1.AdvertisedTopics();
  ASSERT_EQ(advertisedTopics.size(), 1u);
  EXPECT_EQ(advertisedTopics.at(0), g_topic);

  EXPECT_TRUE(node2.Advertise<ignition::msgs::Int32>(g_topic));
  advertisedTopics = node2.AdvertisedTopics();
  ASSERT_EQ(advertisedTopics.size(), 1u);
  EXPECT_EQ(advertisedTopics.at(0), g_topic);

  EXPECT_TRUE(node2.Subscribe(g_topic, cb));
  auto subscribedTopics = node2.SubscribedTopics();
  ASSERT_EQ(subscribedTopics.size(), 1u);
  EXPECT_EQ(subscribedTopics.at(0), g_topic);

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a message by each node.
  EXPECT_TRUE(node1.Publish(g_topic, msg));
  EXPECT_TRUE(node2.Publish(g_topic, msg));

  // Wait some time for the messages to arrive.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was received twice.
  EXPECT_TRUE(cbExecuted);
  EXPECT_EQ(counter, 2);

  reset();
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, PubSubSameThread)
{
  reset();

  ignition::msgs::Int32 msg;
  msg.set_data(data);

  transport::Node node;

  // Advertise an illegal topic.
  EXPECT_FALSE(node.Advertise<ignition::msgs::Int32>("invalid topic"));

  EXPECT_TRUE(node.Advertise<ignition::msgs::Int32>(g_topic));

  // Subscribe to an illegal topic.
  EXPECT_FALSE(node.Subscribe("invalid topic", cb));

  EXPECT_TRUE(node.Subscribe(g_topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Try to publish a message using an invalid topic.
  EXPECT_FALSE(node.Publish("invalid topic", msg));

  // Publish a first message.
  EXPECT_TRUE(node.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);

  reset();

  // Publish a second message on topic.
  EXPECT_TRUE(node.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the data was received.
  EXPECT_TRUE(cbExecuted);

  reset();

  // Unadvertise an illegal topic.
  EXPECT_FALSE(node.Unadvertise("invalid topic"));

  EXPECT_TRUE(node.Unadvertise(g_topic));

  // Publish a third message.
  EXPECT_FALSE(node.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(cbExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief Subscribe to a topic using a lambda function.
TEST(NodeTest, PubSubSameThreadLambda)
{
  reset();

  ignition::msgs::Int32 msg;
  msg.set_data(data);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<ignition::msgs::Int32>(g_topic));

  bool executed = false;
  std::function<void(const ignition::msgs::Int32&)> subCb =
    [&executed](const ignition::msgs::Int32 &_msg)
  {
    EXPECT_EQ(_msg.data(), data);
    executed = true;
  };

  EXPECT_TRUE(node.Subscribe(g_topic, subCb));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a first message.
  EXPECT_TRUE(node.Publish(g_topic, msg));

  EXPECT_TRUE(executed);

  reset();
}

//////////////////////////////////////////////////
/// \brief Use two threads using their own transport nodes. One thread
/// will publish a message, whereas the other thread is subscribed to the topic.
TEST(NodeTest, PubSubTwoThreadsSameTopic)
{
  CreatePubSubTwoThreads();
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
/// \brief Use two different transport node on the same thread. Check that
/// both receive the updates when they are subscribed to the same topic. Check
/// also that when one of the nodes unsubscribes, no longer receives updates.
TEST(NodeTest, PubSubOneThreadTwoSubs)
{
  reset();

  ignition::msgs::Int32 msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise<ignition::msgs::Int32>(g_topic));

  // Subscribe to topic in node1.
  EXPECT_TRUE(node1.Subscribe(g_topic, cb));

  // Subscribe to topic in node2.
  EXPECT_TRUE(node2.Subscribe(g_topic, cb2));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(node1.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was received by node1.
  EXPECT_TRUE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  auto subscribedTopics = node1.SubscribedTopics();
  ASSERT_EQ(subscribedTopics.size(), 1u);
  EXPECT_EQ(subscribedTopics.at(0), g_topic);

  reset();

  // Try to unsubscribe from an invalid topic.
  EXPECT_FALSE(node1.Unsubscribe("invalid topic"));

  // Node1 is not interested in the topic anymore.
  EXPECT_TRUE(node1.Unsubscribe(g_topic));

  // Give some time to receive the unsubscription.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Publish a second message.
  EXPECT_TRUE(node1.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was not received by node1.
  EXPECT_FALSE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  ASSERT_TRUE(node1.SubscribedTopics().empty());

  reset();

  EXPECT_TRUE(node1.Unadvertise(g_topic));

  // Publish a third message.
  EXPECT_FALSE(node1.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Anybody should have received the message.
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(cb2Executed);

  auto subscribedServices = node1.AdvertisedServices();
  ASSERT_TRUE(subscribedServices.empty());

  reset();
}

//////////////////////////////////////////////////
/// \brief Use the transport inside a class and check advertise, subscribe and
/// publish.
TEST(NodeTest, ClassMemberCallbackMessage)
{
  MyTestClass client;
  client.Subscribe();

  // Wait for the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  client.SendSomeData();

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(client.callbackExecuted);
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous and synchronous service calls using member
/// function.
TEST(NodeTest, ClassMemberCallbackService)
{
  MyTestClass client;
  client.TestServiceCall();
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous and synchronous service calls without input
/// using member function.
TEST(NodeTest, ClassMemberCallbackServiceWithoutInput)
{
  MyTestClass client;
  client.TestServiceCallWithoutInput();
}

//////////////////////////////////////////////////
/// \brief Check that the types advertised and published match.
TEST(NodeTest, TypeMismatch)
{
  reset();

  ignition::msgs::Int32 rightMsg;
  ignition::msgs::Vector3d wrongMsg;
  rightMsg.set_data(1);
  wrongMsg.set_x(1);
  wrongMsg.set_y(2);
  wrongMsg.set_z(3);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<ignition::msgs::Int32>(g_topic));

  EXPECT_FALSE(node.Publish(g_topic, wrongMsg));
  EXPECT_TRUE(node.Publish(g_topic, rightMsg));

  reset();
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous service call using free function.
TEST(NodeTest, ServiceCallAsync)
{
  reset();

  ignition::msgs::Int32 req;
  req.set_data(data);

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvEcho));

  EXPECT_TRUE(node.Advertise(g_topic, srvEcho));

  auto advertisedServices = node.AdvertisedServices();
  ASSERT_EQ(advertisedServices.size(), 1u);
  EXPECT_EQ(advertisedServices.at(0), g_topic);

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, response));

  EXPECT_TRUE(node.Request(g_topic, req, response));

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
  reset();

  EXPECT_TRUE(node.Request(g_topic, req, response));

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

  EXPECT_TRUE(node.UnadvertiseSrv(g_topic));

  ASSERT_TRUE(node.AdvertisedServices().empty());

  reset();
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous service call without input using free function.
TEST(NodeTest, ServiceCallWithoutInputAsync)
{
  reset();

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvWithoutInput));

  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutInput));

  auto advertisedServices = node.AdvertisedServices();
  ASSERT_EQ(advertisedServices.size(), 1u);
  EXPECT_EQ(advertisedServices.at(0), g_topic);

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", response));

  EXPECT_TRUE(node.Request(g_topic, response));

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
  reset();

  EXPECT_TRUE(node.Request(g_topic, response));

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

  EXPECT_TRUE(node.UnadvertiseSrv(g_topic));

  ASSERT_TRUE(node.AdvertisedServices().empty());

  reset();
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous service call without waiting for a response
/// \using free function.
TEST(NodeTest, ServiceWithoutOutputCallAsync)
{
  reset();

  transport::Node node;
  ignition::msgs::Int32 req;
  req.set_data(data);

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvWithoutOutput));

  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutOutput));

  auto advertisedServices = node.AdvertisedServices();
  ASSERT_EQ(advertisedServices.size(), 1u);
  EXPECT_EQ(advertisedServices.at(0), g_topic);

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req));

  EXPECT_TRUE(node.Request(g_topic, req));

  int i = 0;
  while (i < 100 && !srvExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call was executed.
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);

  // Try to unadvertise an invalid service.
  EXPECT_FALSE(node.UnadvertiseSrv("invalid service"));

  EXPECT_TRUE(node.UnadvertiseSrv(g_topic));

  ASSERT_TRUE(node.AdvertisedServices().empty());
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous service call using lambdas.
TEST(NodeTest, ServiceCallAsyncLambda)
{
  reset();

  std::function<void(const ignition::msgs::Int32 &, ignition::msgs::Int32 &,
    bool &)> advCb = [](const ignition::msgs::Int32 &_req,
      ignition::msgs::Int32 &_rep, bool &_result)
  {
    EXPECT_EQ(_req.data(), data);
    _rep.set_data(_req.data());
    _result = true;
  };

  transport::Node node;
  EXPECT_TRUE((node.Advertise<ignition::msgs::Int32,
        ignition::msgs::Int32>(g_topic, advCb)));

  bool executed = false;
  std::function<void(const ignition::msgs::Int32 &, const bool)> reqCb =
    [&executed](const ignition::msgs::Int32 &_rep, const bool _result)
  {
    EXPECT_EQ(_rep.data(), data);
    EXPECT_TRUE(_result);
    executed = true;
  };

  ignition::msgs::Int32 req;
  req.set_data(data);

  EXPECT_TRUE((node.Request(g_topic, req, reqCb)));

  EXPECT_TRUE(executed);

  reset();
}

//////////////////////////////////////////////////
/// \brief Make an asynchronous service call without input using lambdas.
TEST(NodeTest, ServiceCallWithoutInputAsyncLambda)
{
  reset();

  std::function<void(ignition::msgs::Int32 &, bool &)> advCb =
    [](ignition::msgs::Int32 &_rep, bool &_result)
  {
    _rep.set_data(data);
    _result = true;
  };

  transport::Node node;
  EXPECT_TRUE((node.Advertise<ignition::msgs::Int32>(g_topic, advCb)));

  bool executed = false;
  std::function<void(const ignition::msgs::Int32 &, const bool)> reqCb =
    [&executed](const ignition::msgs::Int32 &_rep, const bool _result)
  {
    EXPECT_EQ(_rep.data(), data);
    EXPECT_TRUE(_result);
    executed = true;
  };

  EXPECT_TRUE((node.Request(g_topic, reqCb)));

  EXPECT_TRUE(executed);

  reset();
}

//////////////////////////////////////////////////
/// \Make an asynchronous service call without waiting for response using
/// \lambdas.
TEST(NodeTest, ServiceCallWithoutOutputAsyncLambda)
{
  bool executed = false;

  std::function<void(const ignition::msgs::Int32 &)> advCb =
    [&executed](const ignition::msgs::Int32 &_req)
  {
    EXPECT_EQ(_req.data(), data);
    executed = true;
  };

  transport::Node node;
  EXPECT_TRUE((node.Advertise<ignition::msgs::Int32>(g_topic, advCb)));

  ignition::msgs::Int32 req;
  req.set_data(data);

  EXPECT_TRUE(node.Request(g_topic, req));
  EXPECT_TRUE(executed);
}

//////////////////////////////////////////////////
/// \brief Request multiple service calls at the same time.
TEST(NodeTest, MultipleServiceCallAsync)
{
  reset();

  ignition::msgs::Int32 req;
  req.set_data(data);

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvEcho));

  EXPECT_TRUE(node.Advertise(g_topic, srvEcho));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, response));

  EXPECT_TRUE(node.Request(g_topic, req, response));

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
  reset();

  EXPECT_TRUE(node.Request(g_topic, req, response));
  EXPECT_TRUE(node.Request(g_topic, req, response));
  EXPECT_TRUE(node.Request(g_topic, req, response));

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

  EXPECT_TRUE(node.UnadvertiseSrv(g_topic));

  reset();
}

//////////////////////////////////////////////////
/// \brief Request multiple service calls without input at the same time.
TEST(NodeTest, MultipleServiceCallWithoutInputAsync)
{
  reset();

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvWithoutInput));

  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutInput));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", response));

  EXPECT_TRUE(node.Request(g_topic, response));

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
  reset();

  EXPECT_TRUE(node.Request(g_topic, response));
  EXPECT_TRUE(node.Request(g_topic, response));
  EXPECT_TRUE(node.Request(g_topic, response));

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

  EXPECT_TRUE(node.UnadvertiseSrv(g_topic));

  reset();
}

/// \brief Request multiple service calls without without waiting for a response
/// \ at the same time.
TEST(NodeTest, MultipleServiceWithoutOutputCallAsync)
{
  reset();

  transport::Node node;
  ignition::msgs::Int32 req;
  req.set_data(data);

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvWithoutOutput));

  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutOutput));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req));

  EXPECT_TRUE(node.Request(g_topic, req));

  int i = 0;
  while (i < 100 && !srvExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call was executed.
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);

  // Make another request.
  reset();
  EXPECT_TRUE(node.Request(g_topic, req));
  EXPECT_TRUE(node.Request(g_topic, req));
  EXPECT_TRUE(node.Request(g_topic, req));

  i = 0;
  while (i < 100 && counter < 3)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call was executed.
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 3);

  // Try to unadvertise an invalid service.
  EXPECT_FALSE(node.UnadvertiseSrv("invalid service"));

  EXPECT_TRUE(node.UnadvertiseSrv(g_topic));
}

//////////////////////////////////////////////////
/// \brief Make a synchronous service call.
TEST(NodeTest, ServiceCallSync)
{
  reset();

  ignition::msgs::Int32 req;
  ignition::msgs::Int32 rep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvEcho));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, timeout, rep, result));

  EXPECT_TRUE(node.Request(g_topic, req, timeout, rep, result));

  // Check that the service call response was executed.
  EXPECT_TRUE(result);
  EXPECT_EQ(rep.data(), req.data());

  reset();
}

//////////////////////////////////////////////////
/// \brief Make a synchronous service call without input.
TEST(NodeTest, ServiceCallWithoutInputSync)
{
  reset();

  ignition::msgs::Int32 rep;
  bool result;
  unsigned int timeout = 1000;

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutInput));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", timeout, rep, result));

  EXPECT_TRUE(node.Request(g_topic, timeout, rep, result));

  // Check that the service call response was executed.
  EXPECT_TRUE(result);
  EXPECT_EQ(rep.data(), data);

  reset();
}

//////////////////////////////////////////////////
/// \brief Check a timeout in a synchronous service call.
TEST(NodeTest, ServiceCallSyncTimeout)
{
  reset();

  ignition::msgs::Int32 req;
  ignition::msgs::Int32 rep;
  bool result;
  int64_t timeout = 1000;

  req.set_data(data);

  transport::Node node;

  auto t1 = std::chrono::system_clock::now();
  bool executed = node.Request(g_topic, req, static_cast<unsigned int>(timeout),
      rep, result);
  auto t2 = std::chrono::system_clock::now();

  int64_t elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was close to the timeout.
  auto diff = std::max(elapsed, timeout) - std::min(elapsed, timeout);
  EXPECT_LE(diff, 10);

  // Check that the service call response was not executed.
  EXPECT_FALSE(executed);

  reset();
}

//////////////////////////////////////////////////
/// \brief Check a timeout in a synchronous service call without input.
TEST(NodeTest, ServiceCallWithoutInputSyncTimeout)
{
  reset();

  ignition::msgs::Int32 rep;
  bool result;
  int64_t timeout = 1000;

  transport::Node node;

  auto t1 = std::chrono::system_clock::now();
  bool executed = node.Request(g_topic, static_cast<unsigned int>(timeout),
      rep, result);
  auto t2 = std::chrono::system_clock::now();

  int64_t elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was close to the timeout.
  auto diff = std::max(elapsed, timeout) - std::min(elapsed, timeout);
  EXPECT_LE(diff, 10);

  // Check that the service call response was not executed.
  EXPECT_FALSE(executed);

  reset();
}

//////////////////////////////////////////////////
/// \brief Create a publisher that sends messages "forever". This function will
/// be used emiting a SIGINT or SIGTERM signal, to make sure that the transport
/// library captures the signals, stop all the tasks and signal the event with
/// the method Interrupted().
void createInfinitePublisher()
{
  reset();

  ignition::msgs::Int32 msg;
  msg.set_data(data);
  transport::Node node;

  EXPECT_TRUE(node.Advertise<ignition::msgs::Int32>(g_topic));

  auto i = 0;
  bool exitLoop = false;
  while (!exitLoop)
  {
    EXPECT_TRUE(node.Publish(g_topic, msg));
    ++i;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    {
      std::lock_guard<std::mutex> lock(exitMutex);
      if (terminatePub)
        exitLoop = true;
    }
  }

  EXPECT_LT(i, 200);

  reset();
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

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::raise(SIGINT);

  if (thread.joinable())
    thread.join();
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

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::raise(SIGTERM);

  if (thread.joinable())
    thread.join();
}

//////////////////////////////////////////////////
/// \brief Check that a message is not published if the type does not match
/// the type advertised.
TEST(NodeTest, PubSubWrongTypesOnPublish)
{
  reset();

  ignition::msgs::Int32 msg;
  msg.set_data(data);
  ignition::msgs::Vector3d msgV;
  msgV.set_x(1);
  msgV.set_y(2);
  msgV.set_z(3);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<ignition::msgs::Int32>(g_topic));

  EXPECT_TRUE(node.Subscribe(g_topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send a message with a wrong type.
  EXPECT_FALSE(node.Publish(g_topic, msgV));

  // Check that the message was not received.
  EXPECT_FALSE(cbExecuted);

  reset();

  // Publish a second message on topic.
  EXPECT_TRUE(node.Publish(g_topic, msg));

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

  ignition::msgs::Vector3d msgV;
  msgV.set_x(1);
  msgV.set_y(2);
  msgV.set_z(3);

  transport::Node node;

  EXPECT_TRUE(node.Advertise<ignition::msgs::Vector3d>(g_topic));

  EXPECT_TRUE(node.Subscribe(g_topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Send a message with a wrong type.
  EXPECT_TRUE(node.Publish(g_topic, msgV));

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

  ignition::msgs::Int32 msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise<ignition::msgs::Int32>(g_topic));

  // Good subscriber.
  EXPECT_TRUE(node1.Subscribe(g_topic, cb));

  // Bad subscriber: cbVector does not match the types advertised by node1.
  EXPECT_TRUE(node2.Subscribe(g_topic, cbVector));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(node1.Publish(g_topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the message was received by node1.
  EXPECT_TRUE(cbExecuted);

  // Check that the message was not received by node2.
  EXPECT_FALSE(cbVectorExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and a service requester. The
/// requester uses a wrong type for the request argument. The test should verify
/// that the service call does not succeed.
TEST(NodeTest, SrvRequestWrongReq)
{
  reset();

  ignition::msgs::Vector3d wrongReq;
  ignition::msgs::Int32 rep;
  bool result;
  unsigned int timeout = 1000;

  wrongReq.set_x(1);
  wrongReq.set_y(2);
  wrongReq.set_z(3);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvEcho));

  // Request an asynchronous service call with wrong type in the request.
  EXPECT_TRUE(node.Request(g_topic, wrongReq, response));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(responseExecuted);

  // Request a synchronous service call with wrong type in the request.
  EXPECT_FALSE(node.Request(g_topic, wrongReq, timeout, rep, result));

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and a service requester. The
/// requester uses a wrong type for the response argument. The test should
/// verify that the service call does not succeed.
TEST(NodeTest, SrvRequestWrongRep)
{
  reset();

  ignition::msgs::Int32 req;
  ignition::msgs::Vector3d wrongRep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvEcho));

  // Request an asynchronous service call with wrong type in the response.
  EXPECT_TRUE(node.Request(g_topic, req, wrongResponse));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(wrongResponseExecuted);

  // Request a synchronous service call with wrong type in the response.
  EXPECT_FALSE(node.Request(g_topic, req, timeout, wrongRep, result));

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service that doesn't accept input parameters. The
/// service requester uses a wrong type for the response argument. The test
/// should verify that the service call does not succeed.
TEST(NodeTest, SrvWithoutInputRequestWrongRep)
{
  reset();

  ignition::msgs::Vector3d wrongRep;
  bool result;
  unsigned int timeout = 1000;

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutInput));

  // Request an asynchronous service call with wrong type in the response.
  EXPECT_TRUE(node.Request(g_topic, wrongResponse));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(wrongResponseExecuted);

  // Request a synchronous service call with wrong type in the response.
  EXPECT_FALSE(node.Request(g_topic, timeout, wrongRep, result));

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service responser and two service requesters. The
/// service requesters use incorrect types in some of the requests. The test
/// should verify that a response is received only when the appropriate types
/// are used.
TEST(NodeTest, SrvTwoRequestsOneWrong)
{
  reset();

  ignition::msgs::Int32 req;
  ignition::msgs::Int32 goodRep;
  ignition::msgs::Vector3d badRep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvEcho));

  // Request service calls with wrong types in the response.
  EXPECT_FALSE(node.Request(g_topic, req, timeout, badRep, result));
  EXPECT_TRUE(node.Request(g_topic, req, wrongResponse));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(wrongResponseExecuted);

  // Valid service requests.
  EXPECT_TRUE(node.Request(g_topic, req, timeout, goodRep, result));
  EXPECT_TRUE(node.Request(g_topic, req, response));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_TRUE(responseExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief This test spawns a service that doesn't accept input parameters. The
/// service requesters use incorrect types in some of the requests. The test
/// should verify that a response is received only when the appropriate types
/// are used.
TEST(NodeTest, SrvWithoutInputTwoRequestsOneWrong)
{
  reset();

  ignition::msgs::Int32 goodRep;
  ignition::msgs::Vector3d badRep;
  bool result;
  unsigned int timeout = 1000;

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutInput));

  // Request service calls with wrong types in the response.
  EXPECT_FALSE(node.Request(g_topic, timeout, badRep, result));
  EXPECT_TRUE(node.Request(g_topic, wrongResponse));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(wrongResponseExecuted);

  // Valid service requests.
  EXPECT_TRUE(node.Request(g_topic, timeout, goodRep, result));
  EXPECT_TRUE(node.Request(g_topic, response));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_TRUE(responseExecuted);

  reset();
}

//////////////////////////////////////////////////
/// \brief This test creates two nodes and advertises some topics. The test
/// verifies that TopicList() returns the list of all the topics advertised.
TEST(NodeTest, TopicList)
{
  std::vector<std::string> topics;
  transport::Node node1;
  transport::Node node2;

  node1.Advertise<ignition::msgs::Int32>("topic1");
  node2.Advertise<ignition::msgs::Int32>("topic2");

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

  node.Advertise(g_topic, srvEcho);

  node.ServiceList(services);
  EXPECT_EQ(services.size(), 1u);
  services.clear();

  auto start = std::chrono::steady_clock::now();
  node.ServiceList(services);
  auto end = std::chrono::steady_clock::now();
  EXPECT_EQ(services.size(), 1u);

  // The first TopicList() call might block if the discovery is still
  // initializing (it may happen if we run this test alone).
  // However, the second call should never block.
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
/// and is able to terminate.
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
