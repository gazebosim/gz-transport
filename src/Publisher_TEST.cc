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
#include <string>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Publisher.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

// Global constants.
static const std::string             Topic          = "/topic";
static const std::string             Addr           = "tcp://myAddress";
static const std::string             PUuid          = "processUUID";
static const std::string             NUuid          = "nodeUUID";
static const Scope_t                 Scope          = Scope_t::ALL;
static       AdvertiseOptions        Opts1;
static       AdvertiseMessageOptions MsgOpts1;
static       AdvertiseServiceOptions SrvOpts1;
static const std::string             Ctrl           = "controlAddress";
static const std::string             SocketId       = "socketId";
static const std::string             MsgTypeName    = "MessageType";
static const std::string             ReqTypeName    = "RequestType";
static const std::string             RepTypeName    = "ResponseType";

static const std::string             NewTopic       = "/newTopic";
static const std::string             NewAddr        = "tcp://anotherAddress";
static const std::string             NewPUuid       = "processUUID2";
static const std::string             NewNUuid       = "nodeUUID2";
static const Scope_t                 NewScope       = Scope_t::HOST;
static       AdvertiseOptions        Opts2;
static       AdvertiseMessageOptions MsgOpts2;
static       AdvertiseServiceOptions SrvOpts2;
static const std::string             NewCtrl        = "controlAddress2";
static const std::string             NewSocketId    = "socketId2";
static const std::string             NewMsgTypeName = "MessageType2";
static const std::string             NewReqTypeName = "RequestType2";
static const std::string             NewRepTypeName = "ResponseType2";

//////////////////////////////////////////////////
/// \brief Initalize some global variables.
void init()
{
  Opts1.SetScope(Scope);
  Opts2.SetScope(NewScope);
  MsgOpts1.SetScope(Scope);
  MsgOpts1.SetMsgsPerSec(10u);
  MsgOpts2.SetScope(NewScope);
  MsgOpts2.SetMsgsPerSec(20u);
  SrvOpts1.SetScope(Scope);
  SrvOpts2.SetScope(NewScope);
}

//////////////////////////////////////////////////
/// \brief Check the Publisher accessors.
TEST(PublisherTest, Publisher)
{
  init();

  Publisher publisher(Topic, Addr, PUuid, NUuid, Opts1);
  EXPECT_EQ(publisher.Topic(),   Topic);
  EXPECT_EQ(publisher.Addr(),    Addr);
  EXPECT_EQ(publisher.PUuid(),   PUuid);
  EXPECT_EQ(publisher.NUuid(),   NUuid);
  EXPECT_EQ(publisher.Options(), Opts1);
  size_t msgLength = sizeof(uint16_t) + publisher.Topic().size() +
    sizeof(uint16_t) + publisher.Addr().size() +
    sizeof(uint16_t) + publisher.PUuid().size() +
    sizeof(uint16_t) + publisher.NUuid().size() +
    sizeof(uint8_t);
  EXPECT_EQ(publisher.MsgLength(), msgLength);

  std::cout << publisher << std::endl;

  Publisher pub2(publisher);
  EXPECT_TRUE(publisher == pub2);
  EXPECT_FALSE(publisher != pub2);
  msgLength = sizeof(uint16_t) + pub2.Topic().size() +
              sizeof(uint16_t) + pub2.Addr().size()  +
              sizeof(uint16_t) + pub2.PUuid().size() +
              sizeof(uint16_t) + pub2.NUuid().size() +
              sizeof(uint8_t);
  EXPECT_EQ(pub2.MsgLength(), msgLength);

  // Modify the publisher's member variables.
  publisher.SetTopic(NewTopic);
  publisher.SetAddr(NewAddr);
  publisher.SetPUuid(NewPUuid);
  publisher.SetNUuid(NewNUuid);
  publisher.SetOptions(Opts2);

  EXPECT_EQ(publisher.Topic(),   NewTopic);
  EXPECT_EQ(publisher.Addr(),    NewAddr);
  EXPECT_EQ(publisher.PUuid(),   NewPUuid);
  EXPECT_EQ(publisher.NUuid(),   NewNUuid);
  EXPECT_EQ(publisher.Options(), Opts2);
  msgLength = sizeof(uint16_t) + publisher.Topic().size() +
              sizeof(uint16_t) + publisher.Addr().size() +
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
  Publisher publisher(Topic, Addr, PUuid, NUuid, Opts1);

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

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the MessagePublisher accessors.
TEST(PublisherTest, MessagePublisher)
{
  init();

  MessagePublisher pub1(Topic, Addr, Ctrl, PUuid, NUuid, MsgTypeName, MsgOpts1);
  EXPECT_EQ(pub1.Topic(),       Topic);
  EXPECT_EQ(pub1.Addr(),        Addr);
  EXPECT_EQ(pub1.Ctrl(),        Ctrl);
  EXPECT_EQ(pub1.PUuid(),       PUuid);
  EXPECT_EQ(pub1.NUuid(),       NUuid);
  EXPECT_EQ(pub1.MsgTypeName(), MsgTypeName);
  EXPECT_EQ(pub1.Options(),     MsgOpts1);
  size_t msgLength = pub1.Publisher::MsgLength() - Opts1.MsgLength() +
    sizeof(uint16_t) + pub1.Ctrl().size()        +
    sizeof(uint16_t) + pub1.MsgTypeName().size() +
    MsgOpts1.MsgLength();
  EXPECT_EQ(pub1.MsgLength(), msgLength);

  MessagePublisher pub2(pub1);
  EXPECT_TRUE(pub1 == pub2);
  EXPECT_FALSE(pub1 != pub2);
  msgLength = pub2.Publisher::MsgLength() - Opts1.MsgLength() +
    sizeof(uint16_t) + pub2.Ctrl().size()        +
    sizeof(uint16_t) + pub2.MsgTypeName().size() +
    MsgOpts1.MsgLength();
  EXPECT_EQ(pub2.MsgLength(), msgLength);

  // Modify the publisher's member variables.
  pub1.SetTopic(NewTopic);
  pub1.SetAddr(NewAddr);
  pub1.SetCtrl(NewCtrl);
  pub1.SetPUuid(NewPUuid);
  pub1.SetNUuid(NewNUuid);
  pub1.SetMsgTypeName(NewMsgTypeName);
  pub1.SetOptions(MsgOpts2);

  EXPECT_EQ(pub1.Topic(),       NewTopic);
  EXPECT_EQ(pub1.Addr(),        NewAddr);
  EXPECT_EQ(pub1.Ctrl(),        NewCtrl);
  EXPECT_EQ(pub1.PUuid(),       NewPUuid);
  EXPECT_EQ(pub1.NUuid(),       NewNUuid);
  EXPECT_EQ(pub1.MsgTypeName(), NewMsgTypeName);
  EXPECT_EQ(pub1.Options(),     MsgOpts2);
  msgLength = pub1.Publisher::MsgLength() - Opts1.MsgLength() +
    sizeof(uint16_t) + pub1.Ctrl().size()        +
    sizeof(uint16_t) + pub1.MsgTypeName().size() +
    MsgOpts2.MsgLength();;
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
  MessagePublisher publisher(Topic, Addr, Ctrl, PUuid, NUuid, MsgTypeName,
    MsgOpts2);

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
/// \brief Check the ServicePublisher accessors.
TEST(PublisherTest, ServicePublisher)
{
  init();

  ServicePublisher pub1(Topic, Addr, SocketId, PUuid, NUuid,
    ReqTypeName, RepTypeName, SrvOpts1);
  EXPECT_EQ(pub1.Topic(), Topic);
  EXPECT_EQ(pub1.Addr(),  Addr);
  EXPECT_EQ(pub1.SocketId(), SocketId);
  EXPECT_EQ(pub1.PUuid(), PUuid);
  EXPECT_EQ(pub1.NUuid(), NUuid);
  EXPECT_EQ(pub1.ReqTypeName(), ReqTypeName);
  EXPECT_EQ(pub1.RepTypeName(), RepTypeName);
  EXPECT_EQ(pub1.Options().Scope(), Scope);
  size_t msgLength = pub1.Publisher::MsgLength() - Opts1.MsgLength() +
    sizeof(uint16_t) + pub1.SocketId().size()    +
    sizeof(uint16_t) + pub1.ReqTypeName().size() +
    sizeof(uint16_t) + pub1.RepTypeName().size() +
    SrvOpts1.MsgLength();
  EXPECT_EQ(pub1.MsgLength(), msgLength);

  ServicePublisher pub2(pub1);
  EXPECT_TRUE(pub1 == pub2);
  EXPECT_FALSE(pub1 != pub2);
  msgLength = pub2.Publisher::MsgLength() - Opts1.MsgLength() +
    sizeof(uint16_t) + pub2.SocketId().size()    +
    sizeof(uint16_t) + pub2.ReqTypeName().size() +
    sizeof(uint16_t) + pub2.RepTypeName().size() +
    SrvOpts1.MsgLength();
  EXPECT_EQ(pub2.MsgLength(), msgLength);

  // Modify the publisher's member variables.
  pub1.SetTopic(NewTopic);
  pub1.SetAddr(NewAddr);
  pub1.SetSocketId(NewSocketId);
  pub1.SetPUuid(NewPUuid);
  pub1.SetNUuid(NewNUuid);
  pub1.SetReqTypeName(NewReqTypeName);
  pub1.SetRepTypeName(NewRepTypeName);
  pub1.SetOptions(SrvOpts2);

  EXPECT_EQ(pub1.Topic(),       NewTopic);
  EXPECT_EQ(pub1.Addr(),        NewAddr);
  EXPECT_EQ(pub1.SocketId(),    NewSocketId);
  EXPECT_EQ(pub1.PUuid(),       NewPUuid);
  EXPECT_EQ(pub1.NUuid(),       NewNUuid);
  EXPECT_EQ(pub1.ReqTypeName(), NewReqTypeName);
  EXPECT_EQ(pub1.RepTypeName(), NewRepTypeName);
  EXPECT_EQ(pub1.Options(),     SrvOpts2);
  msgLength = pub1.Publisher::MsgLength() - Opts1.MsgLength() +
    sizeof(uint16_t) + pub1.SocketId().size()    +
    sizeof(uint16_t) + pub1.ReqTypeName().size() +
    sizeof(uint16_t) + pub1.RepTypeName().size() +
    SrvOpts2.MsgLength();
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
  ServicePublisher publisher(Topic, Addr, SocketId, PUuid, NUuid,
    ReqTypeName, RepTypeName, SrvOpts2);

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
