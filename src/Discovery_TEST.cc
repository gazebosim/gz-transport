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

#include <uuid/uuid.h>
#include <chrono>
#include <memory>
#include <string>
#include "gtest/gtest.h"
#include "ignition/transport/Discovery.hh"

using namespace ignition;

// Global variables used for multiple tests.
std::string topic = "foo";
std::string localAddr1 = "tcp://127.0.0.1:12345";
std::string controlAddr1 = "tcp://127.0.0.1:12346";
std::string localAddr2 = "tcp://127.0.0.1:12347";
std::string controlAddr2  = "tcp://127.0.0.1:12348";
std::string uuid1Str = "";
std::string uuidNode1Str = "";
std::string uuid2Str = "";
std::string uuidNode2Str = "";
bool discoverResponseExecuted = false;
bool discoverResponseExecutedMF = false;
bool disconnectResponseExecuted = false;
bool disconnectResponseExecutedMF = false;

void setupUUIDs(uuid_t & _uuid1, uuid_t & _uuid2)
{
  uuid_generate(_uuid1);
  uuid1Str = transport::GetGuidStr(_uuid1);

  uuid_generate(_uuid2);
  uuid2Str = transport::GetGuidStr(_uuid2);
}


//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDiscoveryResponse(const std::string &_topic, const std::string &_addr,
  const std::string &_ctrl, const std::string &_procUuid)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_addr, localAddr1);
  EXPECT_EQ(_ctrl, controlAddr1);
  EXPECT_EQ(_procUuid, uuid1Str);
  discoverResponseExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDisconnectResponse(const std::string &_topic, const std::string &_addr,
  const std::string &_ctrl, const std::string &_procUuid)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_addr, localAddr1);
  EXPECT_EQ(_ctrl, controlAddr1);
  EXPECT_EQ(_procUuid, uuid1Str);
  disconnectResponseExecuted = true;
}

//////////////////////////////////////////////////
/// \brief A class for testing subscription passing a member function
/// as a callback.
class MyClass
{
  /// \brief Class constructor.
  public: MyClass(const uuid_t &_uuid)
  {
    this->discov.reset(new transport::Discovery(_uuid));
  }

  /// \brief Class destructor.
  public: virtual ~MyClass()
  {
  }

  /// \brief Register a member function as a discovery callback.
  public: void RegisterConnections()
  {
    this->discov->RegisterDiscoverResp(&MyClass::OnConnectResponse, this);
  }

  /// \brief Register a member function as a discovery disconnection callback.
  public: void RegisterDisconnections()
  {
    this->discov->RegisterDisconnectResp(&MyClass::OnDisconnectResponse, this);
  }

  /// \brief Member function called each time a discovery update is received.
  public: void OnConnectResponse(const std::string &_topic,
    const std::string &_addr, const std::string &_ctrl,
    const std::string &_procUuid)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_addr, localAddr1);
    EXPECT_EQ(_ctrl, controlAddr1);
    EXPECT_EQ(_procUuid, uuid1Str);
    discoverResponseExecutedMF = true;
  }

  /// \brief Member function called each time a disconnect. update is received.
  public: void OnDisconnectResponse(const std::string &_topic,
    const std::string &_addr, const std::string &_ctrl,
    const std::string &_procUuid)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_addr, localAddr1);
    EXPECT_EQ(_ctrl, controlAddr1);
    EXPECT_EQ(_procUuid, uuid1Str);
    disconnectResponseExecutedMF = true;
  }

  private: std::unique_ptr<transport::Discovery> discov;
};

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestBasicAPI)
{
  int newSilenceInterval = 100;
  int newPollingInterval = 200;
  int newSubInterval = 300;

  uuid_t uuid1;
  uuid_generate(uuid1);
  uuid1Str = transport::GetGuidStr(uuid1);

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);

  discovery1.SetMaxSilenceInterval(newSilenceInterval);
  discovery1.SetPollingInterval(newPollingInterval);
  discovery1.SetSubInterval(newSubInterval);
  EXPECT_EQ(discovery1.GetMaxSilenceInterval(), newSilenceInterval);
  EXPECT_EQ(discovery1.GetPollingInterval(), newPollingInterval);
  EXPECT_EQ(discovery1.GetSubInterval(), newSubInterval);
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestAdvertiseNoResponse)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  transport::Discovery discovery2(uuid2);

  // This should generate discovery traffic but no response on discovery2
  // because there is no callback registered.
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !discoverResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was not received.
  EXPECT_FALSE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestAdvertiseNoResponseMF)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecutedMF = false;
  discoverResponseExecutedMF = false;

  // This should generate discovery traffic but no response on object because
  // there is no callback registered.
  transport::Discovery discovery1(uuid1);
  MyClass object(uuid2);

  // This should trigger a discovery response on discovery2.
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !discoverResponseExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was not received.
  EXPECT_FALSE(discoverResponseExecutedMF);
  EXPECT_FALSE(disconnectResponseExecutedMF);
  disconnectResponseExecutedMF = false;
  discoverResponseExecutedMF = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestAdvertise)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving notifications.
  discovery2.RegisterDiscoverResp(onDiscoveryResponse);

  // This should trigger a discovery response on discovery2.
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !discoverResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_TRUE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestAdvertiseMF)
{
  uuid_t uuid1;
  uuid_generate(uuid1);
  uuid1Str = transport::GetGuidStr(uuid1);

  uuid_t uuid2;
  uuid_generate(uuid2);
  uuid2Str = transport::GetGuidStr(uuid2);

  disconnectResponseExecutedMF = false;
  discoverResponseExecutedMF = false;

  // Create two discovery nodes (one is embedded in an object).
  transport::Discovery discovery1(uuid1);
  MyClass object(uuid2);
  object.RegisterConnections();

  // This should trigger a discovery response on object.
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !discoverResponseExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  EXPECT_TRUE(discoverResponseExecutedMF);
  EXPECT_FALSE(disconnectResponseExecutedMF);
  disconnectResponseExecutedMF = false;
  discoverResponseExecutedMF = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestDiscover)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;

  // Create one discovery node and advertise a topic.
  transport::Discovery discovery1(uuid1);
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  // Wait a while.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Create a second discovery node that did not see the previous ADV message.
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving notifications.
  discovery2.RegisterDiscoverResp(onDiscoveryResponse);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // I should not see any discovery updates
  EXPECT_FALSE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);

  // Request the discovery of a topic.
  discovery2.Discover(topic);

  int i = 0;
  while (i < 100 && !discoverResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_TRUE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  discovery2.Discover(topic);

  // Check that the discovery response was received.
  EXPECT_TRUE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestUnadvertise)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving disconnect notifications.
  discovery2.RegisterDisconnectResp(onDisconnectResponse);

  // This should not trigger a disconnect response on discovery2.
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !disconnectResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that no discovery response was received.
  EXPECT_FALSE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;

  // This should trigger a disconnect response on discovery2.
  discovery1.Unadvertise(topic);

  i = 0;
  while (i < 100 && !disconnectResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_FALSE(discoverResponseExecuted);
  EXPECT_TRUE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestUnadvertiseMF)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecutedMF = false;
  discoverResponseExecutedMF = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  MyClass object(uuid2);

  // Register one callback for receiving disconnect notifications.
  object.RegisterDisconnections();

  // This should not trigger a disconnect response on object.
  discovery1.Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !disconnectResponseExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that no discovery response were received.
  EXPECT_FALSE(discoverResponseExecutedMF);
  EXPECT_FALSE(disconnectResponseExecutedMF);
  disconnectResponseExecutedMF = false;

  // This should trigger a disconnect response on discovery2.
  discovery1.Unadvertise(topic);

  i = 0;
  while (i < 100 && !disconnectResponseExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_FALSE(discoverResponseExecutedMF);
  EXPECT_TRUE(disconnectResponseExecutedMF);
  disconnectResponseExecutedMF = false;
  discoverResponseExecutedMF = false;
}

//////////////////////////////////////////////////
TEST(DiscoveryTest, TestNodeBye)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;

  // Create two discovery nodes.
  std::unique_ptr<transport::Discovery> discovery1(
    new transport::Discovery(uuid1));
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving disconnect notifications.
  discovery2.RegisterDisconnectResp(onDisconnectResponse);

  // This should not trigger a disconnect response on discovery2.
  discovery1->Advertise(topic, localAddr1, controlAddr1);

  int i = 0;
  while (i < 100 && !disconnectResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that no discovery response was received.
  EXPECT_FALSE(discoverResponseExecuted);
  EXPECT_FALSE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;

  // Destroy discovery1. It's destructor should send a BYE message and that
  // should be discovered by discovery2.
  discovery1.reset();

  i = 0;
  while (i < 100 && !disconnectResponseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_FALSE(discoverResponseExecuted);
  EXPECT_TRUE(disconnectResponseExecuted);
  disconnectResponseExecuted = false;
  discoverResponseExecuted = false;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
