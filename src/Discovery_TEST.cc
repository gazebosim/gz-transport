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
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

// Global variables used for multiple tests.
std::string topic        = "foo";
std::string localAddr1   = "tcp://127.0.0.1:12345";
std::string controlAddr1 = "tcp://127.0.0.1:12346";
std::string uuidProc1Str = "";
std::string uuidNode1Str = "UUID-Node-1";
std::string localAddr2   = "tcp://127.0.0.1:12347";
std::string controlAddr2 = "tcp://127.0.0.1:12348";
std::string uuidProc2Str = "";
std::string uuidNode2Str = "UUID-Node-2";
transport::Scope scope = transport::Scope::All;
bool connectionExecuted = false;
bool connectionExecutedMF = false;
bool disconnectionExecuted = false;
bool disconnectionExecutedMF = false;
int counter = 0;

//////////////////////////////////////////////////
/// \brief Helper function to generate two UUIDs and its strings representation.
void setupUUIDs(uuid_t & _uuid1, uuid_t & _uuid2)
{
  uuid_generate(_uuid1);
  uuidProc1Str = ignition::transport::GetGuidStr(_uuid1);

  uuid_generate(_uuid2);
  uuidProc2Str = transport::GetGuidStr(_uuid2);
}

//////////////////////////////////////////////////
/// \brief Function called each time a discovery update is received.
void onDiscoveryResponse(const std::string &_topic, const std::string &_addr,
  const std::string &_ctrl, const std::string &_pUuid,
  const std::string &_nUuid, const transport::Scope &_scope)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_addr, localAddr1);
  EXPECT_EQ(_ctrl, controlAddr1);
  EXPECT_EQ(_pUuid, uuidProc1Str);
  EXPECT_EQ(_nUuid, uuidNode1Str);
  EXPECT_EQ(_scope, scope);
  connectionExecuted = true;
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
  EXPECT_EQ(_pUuid, uuidProc1Str);
  disconnectionExecuted = true;
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
    this->discov->SetConnectionsCb(&MyClass::OnConnectResponse, this);
  }

  /// \brief Register a member function as a discovery disconnection callback.
  public: void RegisterDisconnections()
  {
    this->discov->SetDisconnectionsCb(&MyClass::Ondisconnection, this);
  }

  /// \brief Member function called each time a discovery update is received.
  public: void OnConnectResponse(const std::string &_topic,
    const std::string &_addr, const std::string &_ctrl,
    const std::string &_pUuid, const std::string &_nUuid,
    const transport::Scope &_scope)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_addr, localAddr1);
    EXPECT_EQ(_ctrl, controlAddr1);
    EXPECT_EQ(_pUuid, uuidProc1Str);
    EXPECT_EQ(_nUuid, uuidNode1Str);
    EXPECT_EQ(_scope, scope);
    connectionExecutedMF = true;
  }

  /// \brief Member function called each time a disconnect. update is received.
  public: void Ondisconnection(const std::string &/*_topic*/,
    const std::string &/*_addr*/, const std::string &/*_ctrl*/,
    const std::string &_pUuid, const std::string &/*_nUuid*/,
    const transport::Scope &/*_scope*/)
  {
    EXPECT_EQ(_pUuid, uuidProc1Str);
    disconnectionExecutedMF = true;
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
  unsigned int newHeartbitInterval  = 400;

  uuid_t uuid1;
  uuid_generate(uuid1);
  uuidProc1Str = transport::GetGuidStr(uuid1);

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);

  discovery1.SetSilenceInterval(newSilenceInterval);
  discovery1.SetActivityInterval(newActivityInterval);
  discovery1.SetAdvertiseInterval(newAdvertiseInterval);
  discovery1.SetHeartbitInterval(newHeartbitInterval);
  EXPECT_EQ(discovery1.GetSilenceInterval(), newSilenceInterval);
  EXPECT_EQ(discovery1.GetActivityInterval(), newActivityInterval);
  EXPECT_EQ(discovery1.GetAdvertiseInterval(), newAdvertiseInterval);
  EXPECT_EQ(discovery1.GetHeartbitInterval(), newHeartbitInterval);

  EXPECT_NE(discovery1.GetHostAddr(), "");
}

//////////////////////////////////////////////////
/// \brief Advertise a topic without registering callbacks.
TEST(DiscoveryTest, TestAdvertiseNoResponse)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecuted = false;
  connectionExecuted = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  transport::Discovery discovery2(uuid2);

  // This should generate discovery traffic but no response on discovery2
  // because there is no callback registered.
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !connectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was not received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;
  connectionExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Advertise a topic without registering callbacks.
/// This test uses a discovery object within a class.
TEST(DiscoveryTest, TestAdvertiseNoResponseMF)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecutedMF = false;
  connectionExecutedMF = false;

  // This should generate discovery traffic but no response on object because
  // there is no callback registered.
  transport::Discovery discovery1(uuid1);
  MyClass object(uuid2);

  // This should trigger a discovery response on discovery2.
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !connectionExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was not received.
  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);
  disconnectionExecutedMF = false;
  connectionExecutedMF = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
TEST(DiscoveryTest, TestAdvertise)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecuted = false;
  connectionExecuted = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponse);

  // This should trigger a discovery response on discovery2.
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !connectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;
  connectionExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after an advertise.
/// This test uses a discovery object within a class.
TEST(DiscoveryTest, TestAdvertiseMF)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecutedMF = false;
  connectionExecutedMF = false;

  // Create two discovery nodes (one is embedded in an object).
  transport::Discovery discovery1(uuid1);
  MyClass object(uuid2);
  object.RegisterConnections();

  // This should trigger a discovery response on object.
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !connectionExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  EXPECT_TRUE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);
  disconnectionExecutedMF = false;
  connectionExecutedMF = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the callbacks after a discovery
/// and after register the discovery callback.
TEST(DiscoveryTest, TestDiscover)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecuted = false;
  connectionExecuted = false;

  // Create one discovery node and advertise a topic.
  transport::Discovery discovery1(uuid1);
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  // Wait a while.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Create a second discovery node that did not see the previous ADV message.
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponse);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  // Request the discovery of a topic.
  discovery2.Discover(false, topic);

  int i = 0;
  while (i < 100 && !connectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;
  connectionExecuted = false;

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  discovery2.Discover(false, topic);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;
  connectionExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise.
TEST(DiscoveryTest, TestUnadvertise)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecuted = false;
  connectionExecuted = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving disconnect notifications.
  discovery2.SetDisconnectionsCb(ondisconnection);

  // This should not trigger a disconnect response on discovery2.
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !disconnectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;

  // This should trigger a disconnect response on discovery2.
  discovery1.Unadvertise(topic, uuidNode1Str);

  i = 0;
  while (i < 100 && !disconnectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_TRUE(disconnectionExecuted);
  disconnectionExecuted = false;
  connectionExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// an unadvertise. This test uses a discovery object within a class.
TEST(DiscoveryTest, TestUnadvertiseMF)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecutedMF = false;
  connectionExecutedMF = false;

  // Create two discovery nodes.
  transport::Discovery discovery1(uuid1);
  MyClass object(uuid2);

  // Register one callback for receiving disconnect notifications.
  object.RegisterDisconnections();

  // This should not trigger a disconnect response on object.
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !disconnectionExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that no discovery response were received.
  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_FALSE(disconnectionExecutedMF);
  disconnectionExecutedMF = false;

  // This should trigger a disconnect response on discovery2.
  discovery1.Unadvertise(topic, uuidNode1Str);

  i = 0;
  while (i < 100 && !disconnectionExecutedMF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecutedMF);
  EXPECT_TRUE(disconnectionExecutedMF);
  disconnectionExecutedMF = false;
  connectionExecutedMF = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery triggers the disconnection callback after
/// sending a BYE message (discovery object out of scope).
TEST(DiscoveryTest, TestNodeBye)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecuted = false;
  connectionExecuted = false;

  // Create two discovery nodes.
  std::unique_ptr<transport::Discovery> discovery1(
    new transport::Discovery(uuid1));
  transport::Discovery discovery2(uuid2);

  // Register one callback for receiving disconnect notifications.
  discovery2.SetDisconnectionsCb(ondisconnection);

  // This should not trigger a disconnect response on discovery2.
  discovery1->Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);

  int i = 0;
  while (i < 100 && !disconnectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that no discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  disconnectionExecuted = false;

  // Destroy discovery1. It's destructor should send a BYE message and that
  // should be discovered by discovery2.
  discovery1.reset();

  i = 0;
  while (i < 100 && !disconnectionExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the discovery response was received.
  EXPECT_FALSE(connectionExecuted);
  EXPECT_TRUE(disconnectionExecuted);
  disconnectionExecuted = false;
  connectionExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Check that the discovery detects two publishers advertising the same
/// topic name.
TEST(DiscoveryTest, TestTwoPublishersSameTopic)
{
  uuid_t uuid1, uuid2;
  setupUUIDs(uuid1, uuid2);

  disconnectionExecuted = false;
  connectionExecuted = false;
  counter = 0;

  // Create two discovery nodes and advertise the same topic.
  transport::Discovery discovery1(uuid1);
  discovery1.Advertise(transport::AdvertiseType::Msg, topic, localAddr1,
    controlAddr1, uuidNode1Str, scope);
  transport::Discovery discovery2(uuid2, true);
  discovery2.Advertise(transport::AdvertiseType::Msg, topic, localAddr2,
    controlAddr2, uuidNode2Str, scope);

  // Wait a while.
  // std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Register one callback for receiving notifications.
  discovery2.SetConnectionsCb(onDiscoveryResponseMultiple);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // I should not see any discovery updates
  EXPECT_FALSE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);

  // Request the discovery of a topic.
  discovery2.Discover(false, topic);

  int i = 0;
  while (i < 500 && counter < 2)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the two discovery responses were received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  EXPECT_EQ(counter, 2);
  disconnectionExecuted = false;
  connectionExecuted = false;
  counter = 0;

  // Request again the discovery of a topic. The callback should be executed
  // from the Discover method this time because the topic information should be
  // known.
  discovery2.Discover(false, topic);

  // Check that the discovery response was received.
  EXPECT_TRUE(connectionExecuted);
  EXPECT_FALSE(disconnectionExecuted);
  EXPECT_EQ(counter, 2);
  disconnectionExecuted = false;
  connectionExecuted = false;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
