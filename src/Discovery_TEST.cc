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
#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;
using namespace transport;

// Global constants used for multiple tests.
static const int g_MaxIters = 100;
static const int g_Nap = 10;

// Global variables used for multiple tests.
const int g_msgPort = 11319;
const int g_srvPort = 11320;
std::string g_topic   = testing::getRandomNumber();
std::string g_service = testing::getRandomNumber();
std::string g_addr1   = "tcp://127.0.0.1:12345";
std::string g_ctrl1   = "tcp://127.0.0.1:12346";
std::string g_id1     = "identity1";
std::string g_pUuid1  = transport::Uuid().ToString();
std::string g_nUuid1  = transport::Uuid().ToString();
std::string g_addr2   = "tcp://127.0.0.1:12347";
std::string g_ctrl2   = "tcp://127.0.0.1:12348";
std::string g_id2     = "identity2";
std::string g_pUuid2  = transport::Uuid().ToString();
std::string g_nUuid2  = transport::Uuid().ToString();
transport::Scope_t g_scope = transport::Scope_t::ALL;
bool g_connectionExecuted = false;
bool g_disconnectionExecuted = false;
int g_counter = 0;

/// \brief Helper class to access the protected member variables of Discovery
/// within the tests.
template<typename T> class DiscoveryDerived : public transport::Discovery<T>
{
  // Documentation inherited.
  public: DiscoveryDerived(const std::string &_pUuid,
                           const int _port,
                           const bool _verbose = false)
    : transport::Discovery<T>(_pUuid, _port, _verbose)
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
    EXPECT_EQ(this->activity.find(_pUuid) !=
              this->activity.end(), _expectedActivity);
  };
};

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  g_connectionExecuted = false;
  g_disconnectionExecuted = false;
  g_counter = 0;
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
  if (_publisher.NUuid() != g_nUuid1)
    return;

  EXPECT_EQ(_publisher.Addr(), g_addr1);
  EXPECT_EQ(_publisher.Ctrl(), g_ctrl1);
  EXPECT_EQ(_publisher.PUuid(), g_pUuid1);
  g_connectionExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received. This is
/// used in the case of multiple publishers.
void onDiscoveryResponseMultiple(const transport::MessagePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.Topic() != g_topic)
    return;

  EXPECT_NE(_publisher.Addr(), "");
  EXPECT_NE(_publisher.Ctrl(), "");
  EXPECT_NE(_publisher.PUuid(), "");
  EXPECT_NE(_publisher.NUuid(), "");
  g_connectionExecuted = true;
  ++g_counter;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDisconnection(const transport::MessagePublisher &_publisher)
{
  // This discovery event is not relevant for the test, ignore it.
  if (_publisher.PUuid() != g_pUuid1)
    return;

  g_disconnectionExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Test the setters, getters and basic functions.
TEST(DiscoveryTest, TestBasicAPI)
{
  unsigned int newSilenceInterval    = 100;
  unsigned int newActivityInterval   = 200;
  unsigned int newHeartbeatInterval  = 400;

  // Create a discovery node.
  Discovery<MessagePublisher> discovery(g_pUuid1, g_msgPort);

  discovery.SetSilenceInterval(newSilenceInterval);
  discovery.SetActivityInterval(newActivityInterval);
  discovery.SetHeartbeatInterval(newHeartbeatInterval);

  EXPECT_EQ(discovery.SilenceInterval(), newSilenceInterval);
  EXPECT_EQ(discovery.ActivityInterval(), newActivityInterval);
  EXPECT_EQ(discovery.HeartbeatInterval(), newHeartbeatInterval);

  EXPECT_NE(discovery.HostAddr(), "");
}

//////////////////////////////////////////////////
/// \brief Try to use the discovery features without calling Start().
TEST(DiscoveryTest, WithoutCallingStart)
{
  Discovery<ServicePublisher> discovery(g_pUuid1, g_srvPort);
  ServicePublisher srvPublisher(g_service, g_addr1, g_id1, g_pUuid1, g_nUuid1,
    g_scope, "reqType", "repType");

  EXPECT_FALSE(discovery.Advertise(srvPublisher));
  EXPECT_FALSE(discovery.Discover(g_service));
  EXPECT_FALSE(discovery.Unadvertise(g_service, g_nUuid1));
}

//////////////////////////////////////////////////
/// \brief Advertise a topic without registering callbacks.
TEST(DiscoveryTest, TestAdvertiseNoResponse)
{
  reset();

  // Create two discovery nodes.
  MsgDiscovery discovery1(g_pUuid1, g_msgPort);
  MsgDiscovery discovery2(g_pUuid2, g_msgPort);

  discovery1.Start();
  discovery2.Start();

  // This should generate discovery traffic but no response on discovery2
  // because there is no callback registered.
  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  EXPECT_TRUE(discovery1.Advertise(publisher));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  // Check that the discovery callbacks were not received.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
TEST(DiscoveryTest, TestAdvertise)
{
  reset();

  // Create two discovery nodes simulating they are in different processes.
  transport::Discovery<MessagePublisher> discovery1(g_pUuid1, g_msgPort);
  transport::Discovery<MessagePublisher> Discovery2(g_pUuid2, g_msgPort, true);

  // Register one callback for receiving notifications.
  Discovery2.ConnectionsCb(onDiscoveryResponse);

  discovery1.Start();
  Discovery2.Start();

  // This should trigger a discovery response on discovery2.
  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  EXPECT_TRUE(discovery1.Advertise(publisher));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  EXPECT_TRUE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);

  reset();

  // This should not trigger a discovery response on discovery2. They are in
  // different proccesses and the scope is set to "Process".
  MessagePublisher publisher2("/topic2", g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    Scope_t::PROCESS, "type");
  EXPECT_TRUE(discovery1.Advertise(publisher2));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);

  reset();

  // This should trigger a discovery response on discovery2.
  MessagePublisher publisher3("/topic3", g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    Scope_t::HOST, "type");
  EXPECT_TRUE(discovery1.Advertise(publisher3));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  EXPECT_TRUE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
TEST(DiscoveryTest, TestAdvertiseSameProc)
{
  reset();

  // Create two discovery nodes simulating they are in different processes.
  MsgDiscovery discovery1(g_pUuid1, g_msgPort);
  MsgDiscovery discovery2(g_pUuid1, g_msgPort);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponse);

  discovery1.Start();
  discovery2.Start();

  // This should not trigger a discovery response on discovery2. If the nodes
  // are on the same process, they will not communicate using zeromq.
  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  EXPECT_TRUE(discovery1.Advertise(publisher));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after a discovery
/// and after register the discovery callback.
TEST(DiscoveryTest, TestDiscover)
{
  reset();

  // Create one discovery node and advertise a topic.
  Discovery<MessagePublisher> discovery1(g_pUuid1, g_msgPort);
  discovery1.Start();

  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  EXPECT_TRUE(discovery1.Advertise(publisher));

  // Create a second discovery node that did not see the previous ADV message.
  MsgDiscovery discovery2(g_pUuid2, g_msgPort);
  discovery2.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponse);

  // Request the discovery of a topic.
  EXPECT_TRUE(discovery2.Discover(g_topic));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  // Check that the discovery response was received.
  EXPECT_TRUE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);

  reset();

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  EXPECT_TRUE(discovery2.Discover(g_topic));

  // Check that the discovery response was received.
  EXPECT_TRUE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise.
TEST(DiscoveryTest, TestUnadvertise)
{
  reset();

  // Create two discovery nodes.
  MsgDiscovery discovery1(g_pUuid1, g_msgPort);
  MsgDiscovery discovery2(g_pUuid2, g_msgPort);

  // Register one callback for receiving disconnect notifications.
  discovery2.DisconnectionsCb(onDisconnection);

  discovery1.Start();
  discovery2.Start();

  // This should not trigger a disconnect response on discovery2.
  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  EXPECT_TRUE(discovery1.Advertise(publisher));

  waitForCallback(g_MaxIters, g_Nap, g_disconnectionExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);

  reset();

  // This should trigger a disconnect response on discovery2.
  EXPECT_TRUE(discovery1.Unadvertise(g_topic, g_nUuid1));

  waitForCallback(g_MaxIters, g_Nap, g_disconnectionExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_TRUE(g_disconnectionExecuted);

  // Unadvertise a topic not advertised.
  EXPECT_TRUE(discovery1.Unadvertise(g_topic, g_nUuid1));

  MsgAddresses_M addresses;
  EXPECT_FALSE(discovery2.Publishers(g_topic, addresses));
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// sending a BYE message (discovery object out of scope).
TEST(DiscoveryTest, TestNodeBye)
{
  reset();

  // Create two discovery nodes.
  std::unique_ptr<MsgDiscovery> discovery1(
    new MsgDiscovery(g_pUuid1, g_msgPort));
  MsgDiscovery discovery2(g_pUuid2, g_msgPort);

  // Register one callback for receiving disconnect notifications.
  discovery2.DisconnectionsCb(onDisconnection);

  discovery1->Start();
  discovery2.Start();

  // This should not trigger a disconnect response on discovery2.
  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  EXPECT_TRUE(discovery1->Advertise(publisher));

  waitForCallback(g_MaxIters, g_Nap, g_connectionExecuted);

  // Check that no discovery response was received.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
  g_disconnectionExecuted = false;

  // Destroy discovery1. It's destructor should send a BYE message and that
  // should be discovered by discovery2.
  discovery1.reset();

  waitForCallback(g_MaxIters, g_Nap, g_disconnectionExecuted);

  // Check that the discovery response was received.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_TRUE(g_disconnectionExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that the discovery detects two publishers advertising the same
/// topic name.
TEST(DiscoveryTest, TestTwoPublishersSameTopic)
{
  reset();

  // Create two discovery nodes and advertise the same topic.
  MsgDiscovery discovery1(g_pUuid1, g_msgPort);
  MsgDiscovery discovery2(g_pUuid2, g_msgPort);

  MessagePublisher publisher1(g_topic, g_addr1, g_ctrl1, g_pUuid1, g_nUuid1,
    g_scope, "t");
  MessagePublisher publisher2(g_topic, g_addr2, g_ctrl2, g_pUuid2, g_nUuid2,
    g_scope, "t");

  discovery1.Start();
  discovery2.Start();

  EXPECT_TRUE(discovery1.Advertise(publisher1));
  EXPECT_TRUE(discovery2.Advertise(publisher2));

  // The callbacks should not be triggered but let's wait some time in case
  // something goes wrong.
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // I should not see any discovery updates.
  EXPECT_FALSE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);

  // Register one callback for receiving notifications.
  discovery2.ConnectionsCb(onDiscoveryResponseMultiple);

  // Request the discovery of a topic.
  EXPECT_TRUE(discovery2.Discover(g_topic));

  int i = 0;
  while (i < g_MaxIters && g_counter < 2)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(g_Nap));
    ++i;
  }

  // Check that the two discovery responses were received.
  EXPECT_TRUE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
  EXPECT_EQ(g_counter, 2);

  reset();

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  EXPECT_TRUE(discovery2.Discover(g_topic));

  // Check that the discovery response was received.
  EXPECT_TRUE(g_connectionExecuted);
  EXPECT_FALSE(g_disconnectionExecuted);
  EXPECT_EQ(g_counter, 2);
}

//////////////////////////////////////////////////
/// \brief Check that a discovery service sends messages if there are
/// topics or services advertised in its process.
TEST(DiscoveryTest, TestActivity)
{
  auto proc1Uuid = testing::getRandomNumber();
  auto proc2Uuid = testing::getRandomNumber();
  MessagePublisher publisher(g_topic, g_addr1, g_ctrl1, proc1Uuid, g_nUuid1,
    g_scope, "type");
  ServicePublisher srvPublisher(g_service, g_addr1, g_id1, proc2Uuid,
    g_nUuid2, g_scope, "reqType", "repType");
  DiscoveryDerived<MessagePublisher> discovery1(proc1Uuid, g_msgPort);

  {
    DiscoveryDerived<MessagePublisher> discovery2(proc2Uuid, g_msgPort);

    discovery1.Start();
    discovery2.Start();

    discovery1.Advertise(publisher);

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

  transport::Discovery<MessagePublisher> discovery1(g_pUuid1, g_msgPort);
  EXPECT_EQ(discovery1.HostAddr(), "127.0.0.1");

  // Unset IGN_IP.
  unsetenv("IGN_IP");

  // Restore IGN_IP.
  if (ipEnv)
    setenv("IGN_IP", ipEnv, 1);
}
