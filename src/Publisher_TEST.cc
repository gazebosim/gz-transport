/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#include <cstdint>
#include <iostream>
#include <string>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Publisher.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

// Global constants.
static const std::string             g_topic          = "/topic";
static const std::string             g_addr           = "tcp://myAddress";
static const std::string             g_puuid          = "processUUID";
static const std::string             g_nuuid          = "nodeUUID";
static const Scope_t                 g_scope          = Scope_t::ALL;
static       AdvertiseOptions        g_opts1;
static       AdvertiseMessageOptions g_msgOpts1;
static       AdvertiseServiceOptions g_srvOpts1;
static const std::string             g_ctrl           = "controlAddress";
static const std::string             g_socketId       = "socketId";
static const std::string             g_msgTypeName    = "MessageType";
static const std::string             g_reqTypeName    = "RequestType";
static const std::string             g_repTypeName    = "ResponseType";

static const std::string             g_newTopic       = "/g_newTopic";
static const std::string             g_newAddr        = "tcp://anotherAddress";
static const std::string             g_newPUuid       = "processUUID2";
static const std::string             g_newNUuid       = "nodeUUID2";
static const Scope_t                 g_newScope       = Scope_t::HOST;
static       AdvertiseOptions        g_opts2;
static       AdvertiseMessageOptions g_msgOpts2;
static       AdvertiseServiceOptions g_srvOpts2;
static const std::string             g_newCtrl        = "controlAddress2";
static const std::string             g_newSocketId    = "socketId2";
static const std::string             g_newMsgTypeName = "MessageType2";
static const std::string             g_newReqTypeName = "RequestType2";
static const std::string             g_newRepTypeName = "ResponseType2";

//////////////////////////////////////////////////
/// \brief Initalize some global variables.
void init()
{
  g_opts1.SetScope(g_scope);
  g_opts2.SetScope(g_newScope);
  g_msgOpts1.SetScope(g_scope);
  g_msgOpts1.SetMsgsPerSec(10u);
  g_msgOpts2.SetScope(g_newScope);
  g_msgOpts2.SetMsgsPerSec(20u);
  g_srvOpts1.SetScope(g_scope);
  g_srvOpts2.SetScope(g_newScope);
}

//////////////////////////////////////////////////
/// \brief Check the Publisher accessors.
TEST(PublisherTest, Publisher)
{
  init();

  Publisher publisher(g_topic, g_addr, g_puuid, g_nuuid, g_opts1);
  EXPECT_EQ(publisher.Topic(),   g_topic);
  EXPECT_EQ(publisher.Addr(),    g_addr);
  EXPECT_EQ(publisher.PUuid(),   g_puuid);
  EXPECT_EQ(publisher.NUuid(),   g_nuuid);
  EXPECT_EQ(publisher.Options(), g_opts1);

  size_t msgLength = sizeof(uint16_t) + publisher.Topic().size() +
                     sizeof(uint16_t) + publisher.Addr().size()  +
                     sizeof(uint16_t) + publisher.PUuid().size() +
                     sizeof(uint16_t) + publisher.NUuid().size() +
                     sizeof(uint8_t);
  EXPECT_EQ(publisher.MsgLength(), msgLength);

  // Copy constructor.
  Publisher pub2(publisher);

  // Asignment operator.
  Publisher pub3 = pub2;

  // [In]Equality operators.
  EXPECT_TRUE(publisher == pub3);
  EXPECT_FALSE(publisher != pub3);

  msgLength = sizeof(uint16_t) + pub2.Topic().size() +
              sizeof(uint16_t) + pub2.Addr().size()  +
              sizeof(uint16_t) + pub2.PUuid().size() +
              sizeof(uint16_t) + pub2.NUuid().size() +
              sizeof(uint8_t);
  EXPECT_EQ(pub2.MsgLength(), msgLength);

  // Modify the publisher's member variables.
  publisher.SetTopic(g_newTopic);
  publisher.SetAddr(g_newAddr);
  publisher.SetPUuid(g_newPUuid);
  publisher.SetNUuid(g_newNUuid);
  publisher.SetOptions(g_opts2);

  EXPECT_EQ(publisher.Topic(),   g_newTopic);
  EXPECT_EQ(publisher.Addr(),    g_newAddr);
  EXPECT_EQ(publisher.PUuid(),   g_newPUuid);
  EXPECT_EQ(publisher.NUuid(),   g_newNUuid);
  EXPECT_EQ(publisher.Options(), g_opts2);

  msgLength = sizeof(uint16_t) + publisher.Topic().size() +
              sizeof(uint16_t) + publisher.Addr().size()  +
              sizeof(uint16_t) + publisher.PUuid().size() +
              sizeof(uint16_t) + publisher.NUuid().size() +
              sizeof(uint8_t);
  EXPECT_EQ(publisher.MsgLength(), msgLength);
}

//////////////////////////////////////////////////
/// \brief Check the Publisher Pack()/Unpack().
TEST(PublisherTest, PublisherIO)
{
  init();

  // Try to pack an empty publisher.
  Publisher emptyPublisher;
  std::vector<char> buffer(emptyPublisher.MsgLength());
  EXPECT_EQ(emptyPublisher.Pack(&buffer[0]), 0u);

  // Pack a Publisher.
  Publisher publisher(g_topic, g_addr, g_puuid, g_nuuid, g_opts1);

  buffer.resize(publisher.MsgLength());
  size_t bytes = publisher.Pack(&buffer[0]);
  EXPECT_EQ(bytes, publisher.MsgLength());

  // Unpack the Publisher.
  Publisher otherPublisher;
  otherPublisher.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Publisher remains the same.
  EXPECT_EQ(publisher.Topic(),   otherPublisher.Topic());
  EXPECT_EQ(publisher.Addr(),    otherPublisher.Addr());
  EXPECT_EQ(publisher.PUuid(),   otherPublisher.PUuid());
  EXPECT_EQ(publisher.NUuid(),   otherPublisher.NUuid());
  EXPECT_EQ(publisher.Options(), otherPublisher.Options());

  // Try to pack passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the << operator
TEST(PublisherTest, PublisherStreamInsertion)
{
  init();

  Publisher publisher(g_topic, g_addr, g_puuid, g_nuuid, g_opts2);

  std::ostringstream output;
  output << publisher;
  std::string expectedOutput =
    "Publisher:\n"
    "\tTopic: ["       + g_topic + "]" + "\n"
    "\tAddress: "      + g_addr        + "\n"
    "\tProcess UUID: " + g_puuid       + "\n"
    "\tNode UUID: "    + g_nuuid       + "\n"
    "Advertise options:\n"
    "\tScope: Host\n";
  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the MessagePublisher accessors.
TEST(PublisherTest, MessagePublisher)
{
  init();

  MessagePublisher pub1(g_topic, g_addr, g_ctrl, g_puuid, g_nuuid,
    g_msgTypeName, g_msgOpts1);

  EXPECT_EQ(pub1.Topic(),       g_topic);
  EXPECT_EQ(pub1.Addr(),        g_addr);
  EXPECT_EQ(pub1.Ctrl(),        g_ctrl);
  EXPECT_EQ(pub1.PUuid(),       g_puuid);
  EXPECT_EQ(pub1.NUuid(),       g_nuuid);
  EXPECT_EQ(pub1.MsgTypeName(), g_msgTypeName);
  EXPECT_EQ(pub1.Options(),     g_msgOpts1);

  size_t msgLength = pub1.Publisher::MsgLength() - g_opts1.MsgLength() +
                     sizeof(uint16_t) + pub1.Ctrl().size()             +
                     sizeof(uint16_t) + pub1.MsgTypeName().size()      +
                     g_msgOpts1.MsgLength();
  EXPECT_EQ(pub1.MsgLength(), msgLength);

  // [In]Equality operators.
  MessagePublisher pub2(pub1);
  EXPECT_TRUE(pub1 == pub2);
  EXPECT_FALSE(pub1 != pub2);

  msgLength = pub2.Publisher::MsgLength() - g_opts1.MsgLength() +
              sizeof(uint16_t) + pub2.Ctrl().size()             +
              sizeof(uint16_t) + pub2.MsgTypeName().size()      +
              g_msgOpts1.MsgLength();
  EXPECT_EQ(pub2.MsgLength(), msgLength);

  // Modify the publisher's member variables.
  pub1.SetTopic(g_newTopic);
  pub1.SetAddr(g_newAddr);
  pub1.SetCtrl(g_newCtrl);
  pub1.SetPUuid(g_newPUuid);
  pub1.SetNUuid(g_newNUuid);
  pub1.SetMsgTypeName(g_newMsgTypeName);
  pub1.SetOptions(g_msgOpts2);

  EXPECT_EQ(pub1.Topic(),       g_newTopic);
  EXPECT_EQ(pub1.Addr(),        g_newAddr);
  EXPECT_EQ(pub1.Ctrl(),        g_newCtrl);
  EXPECT_EQ(pub1.PUuid(),       g_newPUuid);
  EXPECT_EQ(pub1.NUuid(),       g_newNUuid);
  EXPECT_EQ(pub1.MsgTypeName(), g_newMsgTypeName);
  EXPECT_EQ(pub1.Options(),     g_msgOpts2);
  msgLength = pub1.Publisher::MsgLength() - g_opts1.MsgLength() +
    sizeof(uint16_t) + pub1.Ctrl().size()        +
    sizeof(uint16_t) + pub1.MsgTypeName().size() +
    g_msgOpts2.MsgLength();;
  EXPECT_EQ(pub1.MsgLength(), msgLength);
}

//////////////////////////////////////////////////
/// \brief Check the MessagePublisher Pack()/Unpack().
TEST(PublisherTest, MessagePublisherIO)
{
  init();

  // Try to pack an empty publisher.
  MessagePublisher emptyPublisher;
  std::vector<char> buffer(emptyPublisher.MsgLength());
  EXPECT_EQ(emptyPublisher.Pack(&buffer[0]), 0u);

  // Pack a Publisher.
  MessagePublisher publisher(g_topic, g_addr, g_ctrl, g_puuid, g_nuuid,
    g_msgTypeName, g_msgOpts2);

  buffer.resize(publisher.MsgLength());
  size_t bytes = publisher.Pack(&buffer[0]);
  EXPECT_EQ(bytes, publisher.MsgLength());

  // Unpack the Publisher.
  MessagePublisher otherPublisher;
  otherPublisher.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Publisher remains the same.
  EXPECT_EQ(publisher.Topic(),       otherPublisher.Topic());
  EXPECT_EQ(publisher.Addr(),        otherPublisher.Addr());
  EXPECT_EQ(publisher.Ctrl(),        otherPublisher.Ctrl());
  EXPECT_EQ(publisher.PUuid(),       otherPublisher.PUuid());
  EXPECT_EQ(publisher.NUuid(),       otherPublisher.NUuid());
  EXPECT_EQ(publisher.MsgTypeName(), otherPublisher.MsgTypeName());
  EXPECT_EQ(publisher.Options(),     otherPublisher.Options());

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the << operator
TEST(PublisherTest, MessagePublisherStreamInsertion)
{
  init();

  MessagePublisher pub(g_topic, g_addr, g_ctrl, g_puuid, g_nuuid, g_msgTypeName,
    g_msgOpts2);

  std::ostringstream output;
  output << pub;
  std::string expectedOutput =
    "Publisher:\n"
    "\tTopic: ["       + g_topic + "]" + "\n"
    "\tAddress: "      + g_addr        + "\n"
    "\tProcess UUID: " + g_puuid       + "\n"
    "\tNode UUID: "    + g_nuuid       + "\n"
    "\tControl address: " + g_ctrl       + "\n"
    "\tMessage type: "    + g_msgTypeName + "\n"
    "Advertise options:\n"
    "\tScope: Host\n"
    "\tThrottled? Yes\n"
    "\tRate: 20 msgs/sec\n";

  EXPECT_EQ(output.str(), expectedOutput);
}

//////////////////////////////////////////////////
/// \brief Check the ServicePublisher accessors.
TEST(PublisherTest, ServicePublisher)
{
  init();

  ServicePublisher pub1(g_topic, g_addr, g_socketId, g_puuid, g_nuuid,
    g_reqTypeName, g_repTypeName, g_srvOpts1);
  EXPECT_EQ(pub1.Topic(), g_topic);
  EXPECT_EQ(pub1.Addr(),  g_addr);
  EXPECT_EQ(pub1.SocketId(), g_socketId);
  EXPECT_EQ(pub1.PUuid(), g_puuid);
  EXPECT_EQ(pub1.NUuid(), g_nuuid);
  EXPECT_EQ(pub1.ReqTypeName(), g_reqTypeName);
  EXPECT_EQ(pub1.RepTypeName(), g_repTypeName);
  EXPECT_EQ(pub1.Options().Scope(), g_scope);
  size_t msgLength = pub1.Publisher::MsgLength() - g_opts1.MsgLength() +
    sizeof(uint16_t) + pub1.SocketId().size()    +
    sizeof(uint16_t) + pub1.ReqTypeName().size() +
    sizeof(uint16_t) + pub1.RepTypeName().size() +
    g_srvOpts1.MsgLength();
  EXPECT_EQ(pub1.MsgLength(), msgLength);

  ServicePublisher pub2(pub1);

  EXPECT_TRUE(pub1 == pub2);
  EXPECT_FALSE(pub1 != pub2);

  msgLength = pub2.Publisher::MsgLength() - g_opts1.MsgLength() +
    sizeof(uint16_t) + pub2.SocketId().size()    +
    sizeof(uint16_t) + pub2.ReqTypeName().size() +
    sizeof(uint16_t) + pub2.RepTypeName().size() +
    g_srvOpts1.MsgLength();
  EXPECT_EQ(pub2.MsgLength(), msgLength);

  // Modify the publisher's member variables.
  pub1.SetTopic(g_newTopic);
  pub1.SetAddr(g_newAddr);
  pub1.SetSocketId(g_newSocketId);
  pub1.SetPUuid(g_newPUuid);
  pub1.SetNUuid(g_newNUuid);
  pub1.SetReqTypeName(g_newReqTypeName);
  pub1.SetRepTypeName(g_newRepTypeName);
  pub1.SetOptions(g_srvOpts2);

  EXPECT_EQ(pub1.Topic(),       g_newTopic);
  EXPECT_EQ(pub1.Addr(),        g_newAddr);
  EXPECT_EQ(pub1.SocketId(),    g_newSocketId);
  EXPECT_EQ(pub1.PUuid(),       g_newPUuid);
  EXPECT_EQ(pub1.NUuid(),       g_newNUuid);
  EXPECT_EQ(pub1.ReqTypeName(), g_newReqTypeName);
  EXPECT_EQ(pub1.RepTypeName(), g_newRepTypeName);
  EXPECT_EQ(pub1.Options(),     g_srvOpts2);
  msgLength = pub1.Publisher::MsgLength() - g_opts1.MsgLength() +
    sizeof(uint16_t) + pub1.SocketId().size()    +
    sizeof(uint16_t) + pub1.ReqTypeName().size() +
    sizeof(uint16_t) + pub1.RepTypeName().size() +
    g_srvOpts2.MsgLength();
  EXPECT_EQ(pub1.MsgLength(), msgLength);
}

//////////////////////////////////////////////////
/// \brief Check the ServicePublisher Pack()/Unpack().
TEST(PublisherTest, ServicePublisherIO)
{
  init();

  // Try to pack an empty publisher.
  ServicePublisher emptyPublisher;
  std::vector<char> buffer(emptyPublisher.MsgLength());
  EXPECT_EQ(emptyPublisher.Pack(&buffer[0]), 0u);

  // Pack a Publisher.
  ServicePublisher publisher(g_topic, g_addr, g_socketId, g_puuid, g_nuuid,
    g_reqTypeName, g_repTypeName, g_srvOpts2);

  buffer.resize(publisher.MsgLength());
  size_t bytes = publisher.Pack(&buffer[0]);
  EXPECT_EQ(bytes, publisher.MsgLength());

  // Unpack the Publisher.
  ServicePublisher otherPublisher;
  otherPublisher.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Publisher remains the same.
  EXPECT_EQ(publisher.Topic(), otherPublisher.Topic());
  EXPECT_EQ(publisher.Addr(),  otherPublisher.Addr());
  EXPECT_EQ(publisher.SocketId(),  otherPublisher.SocketId());
  EXPECT_EQ(publisher.PUuid(), otherPublisher.PUuid());
  EXPECT_EQ(publisher.NUuid(), otherPublisher.NUuid());
  EXPECT_EQ(publisher.ReqTypeName(), otherPublisher.ReqTypeName());
  EXPECT_EQ(publisher.RepTypeName(), otherPublisher.RepTypeName());
  EXPECT_EQ(publisher.Options(), otherPublisher.Options());

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the << operator
TEST(PublisherTest, ServicePublisherStreamInsertion)
{
  init();

  ServicePublisher pub(g_topic, g_addr, g_socketId, g_puuid, g_nuuid,
    g_reqTypeName, g_repTypeName, g_srvOpts1);

  std::ostringstream output;
  output << pub;
  std::string expectedOutput =
    "Publisher:\n"
    "\tTopic: ["       + g_topic + "]" + "\n"
    "\tAddress: "      + g_addr        + "\n"
    "\tProcess UUID: " + g_puuid       + "\n"
    "\tNode UUID: "    + g_nuuid       + "\n"
    "\tSocket ID: "    + g_socketId    + "\n"
    "\tRequest type: "    + g_reqTypeName + "\n"
    "\tResponse type: "    + g_repTypeName + "\n"
    "Advertise options:\n"
    "\tScope: All\n";

  EXPECT_EQ(output.str(), expectedOutput);
}
