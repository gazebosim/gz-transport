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
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

// Global constants used for multiple tests.
static const int MaxIters = 100;
static const int Nap = 10;

// Global variables used for multiple tests.
std::string topic   = testing::getRandomNumber();
std::string service = testing::getRandomNumber();
std::string addr1   = "tcp://127.0.0.1:12345";
std::string ctrl1   = "tcp://127.0.0.1:12346";
std::string id1     = "identity1";
std::string pUuid1  = transport::Uuid().ToString();
std::string nUuid1  = transport::Uuid().ToString();
std::string addr2   = "tcp://127.0.0.1:12347";
std::string ctrl2   = "tcp://127.0.0.1:12348";
std::string id2     = "identity2";
std::string pUuid2  = transport::Uuid().ToString();
std::string nUuid2  = transport::Uuid().ToString();
transport::Scope_t scope = transport::Scope_t::ALL;
bool connectionExecuted = false;
bool connectionExecutedMF = false;
bool disconnectionExecuted = false;
bool disconnectionExecutedMF = false;
bool connectionSrvExecuted = false;
bool connectionSrvExecutedMF = false;
bool disconnectionSrvExecuted = false;
bool disconnectionSrvExecutedMF = false;
int counter = 0;

/// \brief Helper class to access the protected member variables of Discovery
/// within the tests.
class DiscoveryDerived : public transport::Discovery
{
  // Documentation inherited.
  public: DiscoveryDerived(const std::string &_pUuid, bool _verbose = false)
    : transport::Discovery(_pUuid, _verbose)
  {
  }

  /// \brief Check if this discovery node has some activity information about
  /// a given process.
  /// \param[in] _pUuid Process UUID that we want to check.
  /// \param[in] _expectedActivity If true, we expect activity on the process
  /// specified as a parameter. If false, we shouldn't have any activity stored.
  public: void TestActivity(const std::string &_pUuid,
                            const bool _expectedActivity) const
  {
    EXPECT_EQ(this->dataPtr->activity.find(_pUuid) !=
              this->dataPtr->activity.end(), _expectedActivity);
  };
};

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  connectionExecuted = false;
  connectionExecutedMF = false;
  disconnectionExecuted = false;
  disconnectionExecutedMF = false;
  connectionSrvExecuted = false;
  connectionSrvExecutedMF = false;
  disconnectionSrvExecuted = false;
  disconnectionSrvExecutedMF = false;
  counter = 0;
}

//////////////////////////////////////////////////
/// \brief Helper function to wait some time until a callback is executed.
void waitForCallback(int _maxIters, int _sleepTimeIter, const bool &_var)
{
  int i = 0;
  while (i < _maxIters && !_var)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(_sleepTimeIter));
    ++i;
  }
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDiscoveryResponse(const transport::MessagePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.NUuid() != nUuid1)
    return;

  EXPECT_EQ(_publisher.Addr(), addr1);
  EXPECT_EQ(_publisher.Ctrl(), ctrl1);
  EXPECT_EQ(_publisher.PUuid(), pUuid1);
  connectionExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery srv call update is received.
void onDiscoverySrvResponse(const transport::ServicePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.NUuid() != nUuid1)
    return;

  EXPECT_EQ(_publisher.Addr(), addr1);
  EXPECT_EQ(_publisher.Topic(), service);
  EXPECT_EQ(_publisher.PUuid(), pUuid1);
  EXPECT_EQ(_publisher.Scope(), scope);
  connectionSrvExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received. This is
/// used in the case of multiple publishers.
void onDiscoveryResponseMultiple(const transport::MessagePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.Topic() != topic)
    return;

  EXPECT_NE(_publisher.Addr(), "");
  EXPECT_NE(_publisher.Ctrl(), "");
  EXPECT_NE(_publisher.PUuid(), "");
  EXPECT_NE(_publisher.NUuid(), "");
  connectionExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDisconnection(const transport::MessagePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.PUuid() != pUuid1)
    return;

  disconnectionExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDisconnectionSrv(const transport::ServicePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.Topic() != service)
    return;

  EXPECT_EQ(_publisher.PUuid(), pUuid1);
  disconnectionSrvExecuted = true;
}

//////////////////////////////////////////////////
/// \brief A class for testing subscription passing a member function
/// as a callback.
class MyClass
{
  /// \brief Class constructor.
  public: explicit MyClass(const std::string &_pUuid)
  {
    this->discov.reset(new transport::Discovery(_pUuid));
  }

  /// \brief Class destructor.
  public: virtual ~MyClass()
  {
  }

  /// \brief Register a member function as a discovery callback.
  public: void RegisterConnections()
  {
    this->discov->ConnectionsCb(&MyClass::OnConnectResponse, this);
  }

  /// \brief Register a member function as a discovery disconnection callback.
  public: void RegisterDisconnections()
  {
    this->discov->DisconnectionsCb(&MyClass::OnDisconnection, this);
  }

    /// \brief Register a member function as a discovery callback (services).
  public: void RegisterSrvConnections()
  {
    this->discov->ConnectionsSrvCb(&MyClass::OnConnectSrvResponse, this);
  }

  /// \brief Register a member function as a discovery disconnection callback
  /// (services).
  public: void RegisterSrvDisconnections()
  {
    this->discov->DisconnectionsSrvCb(&MyClass::OnDisconnectionSrv, this);
  }

  /// \brief Member function called each time a discovery update is received.
  public: void OnConnectResponse(const transport::MessagePublisher &_publisher)
  {
    // This discovery event is not relevant for the test, ignore it.
    if (_publisher.Topic() != topic)
      return;

    EXPECT_EQ(_publisher.Addr(), addr1);
    EXPECT_EQ(_publisher.Ctrl(), ctrl1);
    EXPECT_EQ(_publisher.PUuid(), pUuid1);
    EXPECT_EQ(_publisher.NUuid(), nUuid1);
    EXPECT_EQ(_publisher.Scope(), scope);
    connectionExecutedMF = true;
  }

  /// \brief Member function called each time a disconnect. update is received.
  public: void OnDisconnection(const transport::MessagePublisher &_publisher)
  {
    // This discovery event is not relevant for the test, ignore it.
    if (_publisher.PUuid() != pUuid1)
      return;

    disconnectionExecutedMF = true;
  }

  /// \brief Member function called each time a discovery update is received
  /// (services).
  public: void OnConnectSrvResponse(
    const transport::ServicePublisher &_publisher)
  {
    // This discovery event is not relevant for the test, ignore it.
    if (_publisher.Topic() != service)
      return;

    EXPECT_EQ(_publisher.Addr(), addr1);
    EXPECT_EQ(_publisher.PUuid(), pUuid1);
    EXPECT_EQ(_publisher.NUuid(), nUuid1);
    EXPECT_EQ(_publisher.Scope(), scope);
    connectionSrvExecutedMF = true;
  }

  /// \brief Member function called each time a disconnect. update is received
  /// (services).
  public: void OnDisconnectionSrv(const transport::ServicePublisher &_publisher)
  {
    // This discovery event is not relevant for the test, ignore it.
    if (_publisher.PUuid() != pUuid1)
      return;

    disconnectionSrvExecutedMF = true;
  }

  /// \brief Start the discovery service.
  public: void Start()
  {
    this->discov->Start();
  }

  // \brief A discovery object.
  private: std::unique_ptr<transport::Discovery> discov;
};

//////////////////////////////////////////////////
/// \brief Test the setters, getters and basic functions.
TEST(DiscoveryTest, TestBasicAPI)
{
  unsigned int newSilenceInterval    = 100;
  unsigned int newActivityInterval   = 200;
  unsigned int newAdvertiseInterval  = 300;
  unsigned int newHeartbeatInterval  = 400;

  // Create a discovery node.
  transport::Discovery discovery(pUuid1);

  discovery.SetSilenceInterval(newSilenceInterval);
  discovery.SetActivityInterval(newActivityInterval);
  discovery.SetAdvertiseInterval(newAdvertiseInterval);
  discovery.SetHeartbeatInterval(newHeartbeatInterval);
  EXPECT_EQ(discovery.SilenceInterval(), newSilenceInterval);
  EXPECT_EQ(discovery.ActivityInterval(), newActivityInterval);
  EXPECT_EQ(discovery.AdvertiseInterval(), newAdvertiseInterval);
  EXPECT_EQ(discovery.HeartbeatInterval(), newHeartbeatInterval);

  EXPECT_NE(discovery.HostAddr(), "");
}

//////////////////////////////////////////////////
/// \brief Try to use the discovery features without calling Start().
TEST(DiscoveryTest, WithoutCallingStart)
{
  transport::Discovery discovery(pUuid1);
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");

  transport::ServicePublisher srvPublisher(service, addr1, id1, pUuid1, nUuid1,
    scope, "reqType", "repType");

  EXPECT_FALSE(discovery.AdvertiseMsg(publisher));
  EXPECT_FALSE(discovery.AdvertiseSrv(srvPublisher));
  EXPECT_FALSE(discovery.DiscoverMsg(topic));
  EXPECT_FALSE(discovery.DiscoverSrv(service));
  EXPECT_FALSE(discovery.UnadvertiseMsg(topic, nUuid1));
  EXPECT_FALSE(discovery.UnadvertiseSrv(service, nUuid1));
}

//////////////////////////////////////////////////
/// \brief Advertise a topic without registering callbacks.
TEST(DiscoveryTest, TestAdvertiseNoResponse)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid2);

  discovery1.Start();
  discovery2.Start();

  // This should generate discovery traffic but no response on discovery2
  // because there is no callback registered.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  // Check that the discovery callbacks were not received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Advertise a topic without registering callbacks.
/// This test uses a discovery object within a class.
TEST(DiscoveryTest, TestAdvertiseNoResponseMF)
{
  reset();

  transport::Discovery discovery1(pUuid1);
  MyClass object(pUuid2);

  discovery1.Start();
  object.Start();

  // This should generate discovery traffic but no response on object because
  // there is no callback registered.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, connectionExecutedMF);

  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
TEST(DiscoveryTest, TestAdvertise)
{
  reset();

  // Create two discovery nodes simulating they are in different processes.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid2, true);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponse);

  discovery1.Start();
  discovery2.Start();

  // This should trigger a discovery response on discovery2.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // This should not trigger a discovery response on discovery2. They are in
  // different proccesses and the scope is set to "Process".
  transport::MessagePublisher publisher2("/topic2", addr1, ctrl1, pUuid1,
    nUuid1, transport::Scope_t::PROCESS, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher2));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // This should trigger a discovery response on discovery2.
  transport::MessagePublisher publisher3("/topic3", addr1, ctrl1, pUuid1,
    nUuid1, transport::Scope_t::HOST, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher3));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
TEST(DiscoveryTest, TestAdvertiseSameProc)
{
  reset();

  // Create two discovery nodes simulating they are in different processes.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid1);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponse);

  discovery1.Start();
  discovery2.Start();

  // This should not trigger a discovery response on discovery2. If the nodes
  // are on the same process, they will not communicate using zeromq.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
/// This test uses a discovery object within a class.
TEST(DiscoveryTest, TestAdvertiseMF)
{
  reset();

  // Create two discovery nodes (one is embedded in an object).
  transport::Discovery discovery1(pUuid1);
  MyClass object(pUuid2);
  object.RegisterConnections();

  discovery1.Start();
  object.Start();

  // This should trigger a discovery response on object.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, connectionExecutedMF);

  EXPECT_TRUE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after a discovery
/// and after register the discovery callback.
TEST(DiscoveryTest, TestDiscover)
{
  reset();

  // Create one discovery node and advertise a topic.
  transport::Discovery discovery1(pUuid1);
  discovery1.Start();

  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  // Create a second discovery node that did not see the previous ADV message.
  transport::Discovery discovery2(pUuid2);
  discovery2.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponse);

  // Request the discovery of a topic.
  EXPECT_TRUE(discovery2.DiscoverMsg(topic));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  EXPECT_TRUE(discovery2.DiscoverMsg(topic));

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise.
TEST(DiscoveryTest, TestUnadvertise)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid2);

  // Register one callback for receiving disconnect notifications.
  discovery2.DisconnectionsCb(onDisconnection);

  discovery1.Start();
  discovery2.Start();

  // This should not trigger a disconnect response on discovery2.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, disconnectionExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // This should trigger a disconnect response on discovery2.
  EXPECT_TRUE(discovery1.UnadvertiseMsg(topic, nUuid1));

  waitForCallback(MaxIters, Nap, disconnectionExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_TRUE(disconnectionExecuted);

  // Unadvertise a topic not advertised.
  EXPECT_TRUE(discovery1.UnadvertiseMsg(topic, nUuid1));

  transport::MsgAddresses_M addresses;
  EXPECT_FALSE(discovery2.MsgPublishers(topic, addresses));
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise. This test uses a discovery object within a class.
TEST(DiscoveryTest, TestUnadvertiseMF)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1);
  MyClass object(pUuid2);

  // Register one callback for receiving disconnect notifications.
  object.RegisterDisconnections();

  discovery1.Start();
  object.Start();

  // This should not trigger a disconnect response on object.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, disconnectionExecutedMF);

  // Check that no discovery response were received.
  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);

  reset();

  // This should trigger a disconnect response on discovery2.
  EXPECT_TRUE(discovery1.UnadvertiseMsg(topic, nUuid1));

  waitForCallback(MaxIters, Nap, disconnectionExecutedMF);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_TRUE(disconnectionExecutedMF);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// sending a BYE message (discovery object out of scope).
TEST(DiscoveryTest, TestNodeBye)
{
  reset();

  // Create two discovery nodes.
  std::unique_ptr<transport::Discovery> discovery1(
    new transport::Discovery(pUuid1));
  transport::Discovery discovery2(pUuid2);

  // Register one callback for receiving disconnect notifications.
  discovery2.DisconnectionsCb(onDisconnection);

  discovery1->Start();
  discovery2.Start();

  // This should not trigger a disconnect response on discovery2.
  transport::MessagePublisher publisher(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  EXPECT_TRUE(discovery1->AdvertiseMsg(publisher));

  waitForCallback(MaxIters, Nap, connectionExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;

  // Destroy discovery1. It's destructor should send a BYE message and that
  // should be discovered by discovery2.
  discovery1.reset();

  waitForCallback(MaxIters, Nap, disconnectionExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_TRUE(disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery detects two publishers advertising the same
/// topic name.
TEST(DiscoveryTest, TestTwoPublishersSameTopic)
{
  reset();

  // Create two discovery nodes and advertise the same topic.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid2);

  transport::MessagePublisher publisher1(topic, addr1, ctrl1, pUuid1, nUuid1,
    scope, "type");
  transport::MessagePublisher publisher2(topic, addr2, ctrl2, pUuid2, nUuid2,
    scope, "type");

  discovery1.Start();
  discovery2.Start();

  EXPECT_TRUE(discovery1.AdvertiseMsg(publisher1));
  EXPECT_TRUE(discovery2.AdvertiseMsg(publisher2));

  // The callbacks should not be triggered but let's wait some time in case
  // something goes wrong.
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // I should not see any discovery updates.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponseMultiple);

  // Request the discovery of a topic.
  EXPECT_TRUE(discovery2.DiscoverMsg(topic));

  int i = 0;
  while (i < MaxIters && counter < 2)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(Nap));
    ++i;
  }

  // Check that the two discovery responses were received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  EXPECT_EQ(counter, 2);

  reset();

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  EXPECT_TRUE(discovery2.DiscoverMsg(topic));

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  EXPECT_EQ(counter, 2);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the srv call callbacks after
/// an advertise.
TEST(DiscoveryTest, TestAdvertiseSrv)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid2);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsSrvCb(onDiscoverySrvResponse);

  discovery1.Start();
  discovery2.Start();

  // This should trigger a discovery srv call response on discovery2.
  transport::ServicePublisher publisher(service, addr1, id1, pUuid1, nUuid1,
    scope, "reqType", "repType");

  EXPECT_TRUE(discovery1.AdvertiseSrv(publisher));

  waitForCallback(MaxIters, Nap, connectionSrvExecuted);

  EXPECT_TRUE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after a service
/// call advertise.
/// This test uses a discovery object within a class.
TEST(DiscoveryTest, TestAdvertiseSrvMF)
{
  reset();

  // Create two discovery nodes (one is embedded in an object).
  transport::Discovery discovery1(pUuid1);
  MyClass object(pUuid2);
  object.RegisterSrvConnections();

  discovery1.Start();
  object.Start();

  // This should trigger a discovery response on object.
  transport::ServicePublisher publisher(service, addr1, id1, pUuid1, nUuid1,
    scope, "reqType", "repType");
  EXPECT_TRUE(discovery1.AdvertiseSrv(publisher));

  waitForCallback(MaxIters, Nap, connectionSrvExecutedMF);

  EXPECT_TRUE(connectionSrvExecutedMF);
  EXPECT_FALSE(disconnectionSrvExecutedMF);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise.
TEST(DiscoveryTest, TestUnadvertiseSrv)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1);
  transport::Discovery discovery2(pUuid2);

  // Register one callback for receiving disconnect  notifications (srv calls).
  discovery2.DisconnectionsSrvCb(onDisconnectionSrv);

  discovery1.Start();
  discovery2.Start();

  // This should not trigger a disconnect response on discovery2.
  transport::ServicePublisher publisher1(service, addr1, id1, pUuid1, nUuid1,
    scope, "reqType", "repType");
  EXPECT_TRUE(discovery1.AdvertiseSrv(publisher1));

  waitForCallback(MaxIters, Nap, disconnectionSrvExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);

  reset();

  // This should trigger a disconnect response on discovery2.
  EXPECT_TRUE(discovery1.UnadvertiseSrv(service, nUuid1));

  waitForCallback(MaxIters, Nap, disconnectionSrvExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionSrvExecuted);
  EXPECT_TRUE(disconnectionSrvExecuted);

  // Unadvertise a topic not advertised.
  EXPECT_TRUE(discovery1.UnadvertiseSrv(service, nUuid1));
  transport::SrvAddresses_M addresses;
  EXPECT_FALSE(discovery2.SrvPublishers(topic, addresses));
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise service. This test uses a discovery object within a class.
TEST(DiscoveryTest, TestUnadvertiseSrvMF)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1);
  MyClass object(pUuid2);

  // Register one callback for receiving disconnect notifications.
  object.RegisterSrvDisconnections();

  discovery1.Start();
  object.Start();

  // This should not trigger a disconnect response on object.
  transport::ServicePublisher publisher(service, addr1, id1, pUuid1, nUuid1,
    scope, "reqType", "repType");
  EXPECT_TRUE(discovery1.AdvertiseSrv(publisher));

  waitForCallback(MaxIters, Nap, disconnectionSrvExecutedMF);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionSrvExecutedMF);
  EXPECT_FALSE(disconnectionSrvExecutedMF);

  reset();

  // This should trigger a disconnect response on discovery2.
  EXPECT_TRUE(discovery1.UnadvertiseSrv(service, nUuid1));

  waitForCallback(MaxIters, Nap, disconnectionSrvExecutedMF);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionSrvExecutedMF);
  EXPECT_TRUE(disconnectionSrvExecutedMF);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery service triggers the callbacks after
/// a discovery and after register the discovery callback.
TEST(DiscoveryTest, TestDiscoverSrv)
{
  reset();

  // Create one discovery node and advertise a service.
  transport::Discovery discovery1(pUuid1);
  discovery1.Start();
  transport::ServicePublisher publisher(service, addr1, id1, pUuid1, nUuid1,
    scope, "reqType", "repType");
  EXPECT_TRUE(discovery1.AdvertiseSrv(publisher));

  // Create a second discovery node that did not see the previous ADVSRV message
  transport::Discovery discovery2(pUuid2);
  discovery2.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates.
  EXPECT_FALSE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsSrvCb(onDiscoverySrvResponse);

  // Request the discovery of a service.
  EXPECT_TRUE(discovery2.DiscoverSrv(service));

  waitForCallback(MaxIters, Nap, connectionSrvExecuted);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);

  reset();

  // Request again the discovery of a service. The callback should be executed
  // from the Discover method this time because the service information should
  // be known.
  EXPECT_TRUE(discovery2.DiscoverSrv(service));

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that a discovery service sends messages if there are
/// topics or services advertised in its process.
TEST(DiscoveryTest, TestActivity)
{
  auto proc1Uuid = testing::getRandomNumber();
  auto proc2Uuid = testing::getRandomNumber();
  transport::MessagePublisher publisher(topic, addr1, ctrl1, proc1Uuid,
    nUuid1, scope, "type");
  transport::ServicePublisher srvPublisher(service, addr1, id1, proc2Uuid,
    nUuid2, scope, "reqType", "repType");
  DiscoveryDerived discovery1(proc1Uuid);

  {
    DiscoveryDerived discovery2(proc2Uuid);

    discovery1.Start();
    discovery2.Start();

    discovery1.AdvertiseMsg(publisher);

    std::this_thread::sleep_for(std::chrono::milliseconds(
      discovery1.HeartbeatInterval() * 2));

    // We should observe activity from both processes.
    discovery1.TestActivity(proc2Uuid, true);
    discovery2.TestActivity(proc1Uuid, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(
      discovery1.HeartbeatInterval() * 2));
  }

  // We shouldn't observe activity from proc1Uuid2 anymore.
  discovery1.TestActivity(proc2Uuid, false);
}

//////////////////////////////////////////////////
/// \brief Check that a wrong IGN_IP value makes HostAddr() to return 127.0.0.1
TEST(DiscoveryTest, WrongIgnIp)
{
  // Save the current value of IGN_IP environment variable.
  char *ipEnv;
#ifdef _MSC_VER
  size_t sz = 0;
  _dupenv_s(&ipEnv, &sz, "IGN_IP");
#else
  ipEnv = std::getenv("IGN_IP");
#endif

  // Incorrect value for IGN_IP
  setenv("IGN_IP", "127.0.0.0", 1);

  transport::Discovery discovery1(pUuid1);
  EXPECT_EQ(discovery1.HostAddr(), "127.0.0.1");

  // Unset IGN_IP.
  unsetenv("IGN_IP");

  // Restore IGN_IP.
  if (ipEnv)
    setenv("IGN_IP", ipEnv, 1);
}
