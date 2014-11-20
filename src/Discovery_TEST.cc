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
#include <memory>
#include <string>
#include "gtest/gtest.h"
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

// Global constants used for multiple tests.
static const int MaxIters = 100;
static const int Nap = 10;

// Global variables used for multiple tests.
std::string topic   = "/foo";
std::string service = "/service";
std::string addr1   = "tcp://127.0.0.1:12345";
std::string ctrl1   = "tcp://127.0.0.1:12346";
std::string id1     = "identity1";
std::string pUuid1  = "UUID-Proc-1";
std::string nUuid1  = "UUID-Node-1";
std::string addr2   = "tcp://127.0.0.1:12347";
std::string ctrl2   = "tcp://127.0.0.1:12348";
std::string id2     = "identity2";
std::string pUuid2  = "UUID-Proc-2";
std::string nUuid2  = "UUID-Node-2";
transport::Scope scope = transport::Scope::All;
bool connectionExecuted = false;
bool connectionExecutedMF = false;
bool disconnectionExecuted = false;
bool disconnectionExecutedMF = false;
bool connectionSrvExecuted = false;
bool connectionSrvExecutedMF = false;
bool disconnectionSrvExecuted = false;
bool disconnectionSrvExecutedMF = false;
int counter = 0;

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
void onDiscoveryResponse(const std::string &/*_topic*/,
  const std::string &_addr, const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const transport::Scope &/*_scope*/)
{
  EXPECT_EQ(_addr, addr1);
  EXPECT_EQ(_ctrl, ctrl1);
  EXPECT_EQ(_pUuid, pUuid1);
  EXPECT_EQ(_nUuid, nUuid1);
  connectionExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery srv call update is received.
void onDiscoverySrvResponse(const std::string &_service,
  const std::string &_addr, const std::string &/*_ctrl*/,
  const std::string &_pUuid, const std::string &_nUuid,
  const transport::Scope &_scope)
{
  EXPECT_EQ(_service, service);
  EXPECT_EQ(_addr, addr1);
  EXPECT_EQ(_pUuid, pUuid1);
  EXPECT_EQ(_nUuid, nUuid1);
  EXPECT_EQ(_scope, scope);
  connectionSrvExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received. This is
/// used in the case of multiple publishers.
void onDiscoveryResponseMultiple(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl,
  const std::string &_pUuid, const std::string &_nUuid,
  const transport::Scope &/*_scope*/)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_NE(_addr, "");
  EXPECT_NE(_ctrl, "");
  EXPECT_NE(_pUuid, "");
  EXPECT_NE(_nUuid, "");
  connectionExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void ondisconnection(const std::string &/*_topic*/,
  const std::string &/*_addr*/, const std::string &/*_ctrl*/,
  const std::string &_pUuid, const std::string &/*_nUuid*/,
  const transport::Scope &/*_scope*/)
{
  EXPECT_EQ(_pUuid, pUuid1);
  disconnectionExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void ondisconnectionSrv(const std::string &_topic,
  const std::string &/*_addr*/, const std::string &/*_ctrl*/,
  const std::string &_pUuid, const std::string &/*_nUuid*/,
  const transport::Scope &/*_scope*/)
{
  EXPECT_EQ(_topic, service);
  EXPECT_EQ(_pUuid, pUuid1);
  disconnectionSrvExecuted = true;
}

//////////////////////////////////////////////////
/// \brief A class for testing subscription passing a member function
/// as a callback.
class MyClass
{
  /// \brief Class constructor.
  public: MyClass(const std::string &_pUuid)
  {
    this->discov.reset(new transport::Discovery(_pUuid, nullptr, nullptr));
  }

  /// \brief Class destructor.
  public: virtual ~MyClass()
  {
  }

  /// \brief Register a member function as a discovery callback.
  public: void RegisterConnections()
  {
    this->discov->SetConnectionsCb(&MyClass::OnConnectResponse, this);
  }

  /// \brief Register a member function as a discovery disconnection callback.
  public: void RegisterDisconnections()
  {
    this->discov->SetDisconnectionsCb(&MyClass::Ondisconnection, this);
  }

    /// \brief Register a member function as a discovery callback (services).
  public: void RegisterSrvConnections()
  {
    this->discov->SetConnectionsSrvCb(&MyClass::OnConnectSrvResponse, this);
  }

  /// \brief Register a member function as a discovery disconnection callback
  /// (services).
  public: void RegisterSrvDisconnections()
  {
    this->discov->SetDisconnectionsSrvCb(&MyClass::OndisconnectionSrv, this);
  }

  /// \brief Member function called each time a discovery update is received.
  public: void OnConnectResponse(const std::string &_topic,
    const std::string &_addr, const std::string &_ctrl,
    const std::string &_pUuid, const std::string &_nUuid,
    const transport::Scope &_scope)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_addr, addr1);
    EXPECT_EQ(_ctrl, ctrl1);
    EXPECT_EQ(_pUuid, pUuid1);
    EXPECT_EQ(_nUuid, nUuid1);
    EXPECT_EQ(_scope, scope);
    connectionExecutedMF = true;
  }

  /// \brief Member function called each time a disconnect. update is received.
  public: void Ondisconnection(const std::string &/*_topic*/,
    const std::string &/*_addr*/, const std::string &/*_ctrl*/,
    const std::string &_pUuid, const std::string &/*_nUuid*/,
    const transport::Scope &/*_scope*/)
  {
    EXPECT_EQ(_pUuid, pUuid1);
    disconnectionExecutedMF = true;
  }

  /// \brief Member function called each time a discovery update is received
  /// (services).
  public: void OnConnectSrvResponse(const std::string &_service,
    const std::string &_addr, const std::string &/*_ctrl*/,
    const std::string &_pUuid, const std::string &_nUuid,
    const transport::Scope &_scope)
  {
    EXPECT_EQ(_service, service);
    EXPECT_EQ(_addr, addr1);
    EXPECT_EQ(_pUuid, pUuid1);
    EXPECT_EQ(_nUuid, nUuid1);
    EXPECT_EQ(_scope, scope);
    connectionSrvExecutedMF = true;
  }

  /// \brief Member function called each time a disconnect. update is received
  /// (services).
  public: void OndisconnectionSrv(const std::string &/*_service*/,
    const std::string &/*_addr*/, const std::string &/*_ctrl*/,
    const std::string &_pUuid, const std::string &/*_nUuid*/,
    const transport::Scope &/*_scope*/)
  {
    EXPECT_EQ(_pUuid, pUuid1);
    disconnectionSrvExecutedMF = true;
  }

  // \brief A discovery object.
  private: std::unique_ptr<transport::Discovery> discov;
};

//////////////////////////////////////////////////
/// \brief Test the setters, getters and basic functions.
TEST(DiscoveryTest, TestBasicAPI)
{
  unsigned int newSilenceInterval   = 100;
  unsigned int newActivityInterval  = 200;
  unsigned int newAdvertiseInterval = 300;
  unsigned int newHeartbeatInterval  = 400;

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);

  discovery1.SetSilenceInterval(newSilenceInterval);
  discovery1.SetActivityInterval(newActivityInterval);
  discovery1.SetAdvertiseInterval(newAdvertiseInterval);
  discovery1.SetHeartbeatInterval(newHeartbeatInterval);
  EXPECT_EQ(discovery1.GetSilenceInterval(), newSilenceInterval);
  EXPECT_EQ(discovery1.GetActivityInterval(), newActivityInterval);
  EXPECT_EQ(discovery1.GetAdvertiseInterval(), newAdvertiseInterval);
  EXPECT_EQ(discovery1.GetHeartbeatInterval(), newHeartbeatInterval);

  EXPECT_NE(discovery1.GetHostAddr(), "");
}

//////////////////////////////////////////////////
/// \brief Advertise a topic without registering callbacks.
TEST(DiscoveryTest, TestAdvertiseNoResponse)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);

  // This should generate discovery traffic but no response on discovery2
  // because there is no callback registered.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

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

  // This should generate discovery traffic but no response on object because
  // there is no callback registered.
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  MyClass object(pUuid2);

  // This should trigger a discovery response on discovery2.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  transport::Discovery discovery2(pUuid2, nullptr, nullptr, true);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponse);

  // This should trigger a discovery response on discovery2.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

  waitForCallback(MaxIters, Nap, connectionExecuted);

  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // This should not trigger a discovery response on discovery2. They are in
  // different proccesses and the scope is set to "Process".
  discovery1.AdvertiseMsg("/topic2", addr1, ctrl1, nUuid1,
    transport::Scope::Process);

  waitForCallback(MaxIters, Nap, connectionExecuted);

  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // This should trigger a discovery response on discovery2.
  discovery1.AdvertiseMsg("/topic3", addr1, ctrl1, nUuid1,
    transport::Scope::Host);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  transport::Discovery discovery2(pUuid1, nullptr, nullptr);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponse);

  // This should not trigger a discovery response on discovery2. If the nodes
  // are on the same process, they will not communicate using zeromq.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  MyClass object(pUuid2);
  object.RegisterConnections();

  // This should trigger a discovery response on object.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

  // Create a second discovery node that did not see the previous ADV message.
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponse);

  // Request the discovery of a topic.
  discovery2.DiscoverMsg(topic);

  waitForCallback(MaxIters, Nap, connectionExecuted);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  discovery2.DiscoverMsg(topic);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);

  // Register one callback for receiving disconnect notifications.
  discovery2.SetDisconnectionsCb(ondisconnection);

  // This should not trigger a disconnect response on discovery2.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

  waitForCallback(MaxIters, Nap, disconnectionExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  reset();

  // This should trigger a disconnect response on discovery2.
  discovery1.UnadvertiseMsg(topic, nUuid1);

  waitForCallback(MaxIters, Nap, disconnectionExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_TRUE(disconnectionExecuted);

  // Unadvertise a topic not advertised.
  discovery1.UnadvertiseMsg(topic, nUuid1);
  transport::Addresses_M addresses;
  EXPECT_FALSE(discovery2.GetMsgAddresses(topic, addresses));
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise. This test uses a discovery object within a class.
TEST(DiscoveryTest, TestUnadvertiseMF)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  MyClass object(pUuid2);

  // Register one callback for receiving disconnect notifications.
  object.RegisterDisconnections();

  // This should not trigger a disconnect response on object.
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

  waitForCallback(MaxIters, Nap, disconnectionExecutedMF);

  // Check that no discovery response were received.
  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);

  reset();

  // This should trigger a disconnect response on discovery2.
  discovery1.UnadvertiseMsg(topic, nUuid1);

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
    new transport::Discovery(pUuid1, nullptr, nullptr));
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);

  // Register one callback for receiving disconnect notifications.
  discovery2.SetDisconnectionsCb(ondisconnection);

  // This should not trigger a disconnect response on discovery2.
  discovery1->AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  discovery1.AdvertiseMsg(topic, addr1, ctrl1, nUuid1, scope);
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);
  discovery2.AdvertiseMsg(topic, addr2, ctrl2, nUuid2, scope);

  // The callbacks should not be triggered but let's wait some time in case
  // something goes wrong.
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // I should not see any discovery updates.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponseMultiple);

  // Request the discovery of a topic.
  discovery2.DiscoverMsg(topic);

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
  discovery2.DiscoverMsg(topic);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsSrvCb(onDiscoverySrvResponse);

  // This should trigger a discovery srv call response on discovery2.
  discovery1.AdvertiseSrv(service, addr1, id1, nUuid1, scope);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  MyClass object(pUuid2);
  object.RegisterSrvConnections();

  // This should trigger a discovery response on object.
  discovery1.AdvertiseSrv(service, addr1, id1, nUuid1, scope);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);

  // Register one callback for receiving disconnect  notifications (srv calls).
  discovery2.SetDisconnectionsSrvCb(ondisconnectionSrv);

  // This should not trigger a disconnect response on discovery2.
  discovery1.AdvertiseSrv(service, addr1, id1, nUuid1, scope);

  waitForCallback(MaxIters, Nap, disconnectionSrvExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);

  reset();

  // This should trigger a disconnect response on discovery2.
  discovery1.UnadvertiseSrv(service, nUuid1);

  waitForCallback(MaxIters, Nap, disconnectionSrvExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionSrvExecuted);
  EXPECT_TRUE(disconnectionSrvExecuted);

  // Unadvertise a topic not advertised.
  discovery1.UnadvertiseSrv(service, nUuid1);
  transport::Addresses_M addresses;
  EXPECT_FALSE(discovery2.GetMsgAddresses(topic, addresses));
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise service. This test uses a discovery object within a class.
TEST(DiscoveryTest, TestUnadvertiseSrvMF)
{
  reset();

  // Create two discovery nodes.
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  MyClass object(pUuid2);

  // Register one callback for receiving disconnect notifications.
  object.RegisterSrvDisconnections();

  // This should not trigger a disconnect response on object.
  discovery1.AdvertiseSrv(service, addr1, id1, nUuid1, scope);

  waitForCallback(MaxIters, Nap, disconnectionSrvExecutedMF);

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionSrvExecutedMF);
  EXPECT_FALSE(disconnectionSrvExecutedMF);

  reset();

  // This should trigger a disconnect response on discovery2.
  discovery1.UnadvertiseSrv(service, nUuid1);

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
  transport::Discovery discovery1(pUuid1, nullptr, nullptr);
  discovery1.AdvertiseSrv(service, addr1, id1, nUuid1, scope);

  // Create a second discovery node that did not see the previous ADVSRV message
  transport::Discovery discovery2(pUuid2, nullptr, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates.
  EXPECT_FALSE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsSrvCb(onDiscoverySrvResponse);

  // Request the discovery of a service.
  discovery2.DiscoverSrv(service);

  waitForCallback(MaxIters, Nap, connectionSrvExecuted);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);

  reset();

  // Request again the discovery of a service. The callback should be executed
  // from the Discover method this time because the service information should
  // be known.
  discovery2.DiscoverSrv(service);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionSrvExecuted);
  EXPECT_FALSE(disconnectionSrvExecuted);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
