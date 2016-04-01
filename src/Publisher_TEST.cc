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

#include <string>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Publisher.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

// Global constants.
static const std::string Topic       = "/topic";
static const std::string Addr        = "tcp://myAddress";
static const std::string PUuid       = "processUUID";
static const std::string NUuid       = "nodeUUID";
static const Scope_t     Scope       = Scope_t::ALL;
static const std::string Ctrl        = "controlAddress";
static const std::string SocketId    = "socketId";
static const std::string MsgTypeName = "MessageType";
static const std::string ReqTypeName = "RequestType";
static const std::string RepTypeName = "ResponseType";

static const std::string NewTopic       = "/newTopic";
static const std::string NewAddr        = "tcp://anotherAddress";
static const std::string NewPUuid       = "processUUID2";
static const std::string NewNUuid       = "nodeUUID2";
static const Scope_t     NewScope       = Scope_t::HOST;
static const std::string NewCtrl        = "controlAddress2";
static const std::string NewSocketId    = "socketId2";
static const std::string NewMsgTypeName = "MessageType2";
static const std::string NewReqTypeName = "RequestType2";
static const std::string NewRepTypeName = "ResponseType2";

//////////////////////////////////////////////////
/// \brief Check the Publisher accessors.
TEST(PublisherTest, Publisher)
{
  Publisher publisher(Topic, Addr, PUuid, NUuid, Scope);
  EXPECT_EQ(publisher.Topic(), Topic);
  EXPECT_EQ(publisher.Addr(),  Addr);
  EXPECT_EQ(publisher.PUuid(), PUuid);
  EXPECT_EQ(publisher.NUuid(), NUuid);
  EXPECT_EQ(publisher.Scope(), Scope);

  Publisher pub2(publisher);
  EXPECT_TRUE(publisher == pub2);
  EXPECT_FALSE(publisher != pub2);

  // Modify the publisher's member variables.
  publisher.SetTopic(NewTopic);
  publisher.SetAddr(NewAddr);
  publisher.SetPUuid(NewPUuid);
  publisher.SetNUuid(NewNUuid);
  publisher.SetScope(NewScope);

  EXPECT_EQ(publisher.Topic(), NewTopic);
  EXPECT_EQ(publisher.Addr(),  NewAddr);
  EXPECT_EQ(publisher.PUuid(), NewPUuid);
  EXPECT_EQ(publisher.NUuid(), NewNUuid);
  EXPECT_EQ(publisher.Scope(), NewScope);
}

//////////////////////////////////////////////////
/// \brief Check the Publisher Pack()/Unpack().
TEST(PublisherTest, PublisherIO)
{
  // Try to pack an empty publisher.
  Publisher emptyPublisher;
  std::vector<char> buffer(emptyPublisher.MsgLength());
  EXPECT_EQ(emptyPublisher.Pack(&buffer[0]), 0u);

  // Pack a Publisher.
  Publisher publisher(Topic, Addr, PUuid, NUuid, Scope);

  buffer.resize(publisher.MsgLength());
  size_t bytes = publisher.Pack(&buffer[0]);
  EXPECT_EQ(bytes, publisher.MsgLength());

  // Unpack the Publisher.
  Publisher otherPublisher;
  otherPublisher.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Publisher remains the same.
  EXPECT_EQ(publisher.Topic(), otherPublisher.Topic());
  EXPECT_EQ(publisher.Addr(),  otherPublisher.Addr());
  EXPECT_EQ(publisher.PUuid(), otherPublisher.PUuid());
  EXPECT_EQ(publisher.NUuid(), otherPublisher.NUuid());
  EXPECT_EQ(publisher.Scope(), otherPublisher.Scope());

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the MessagePublisher accessors.
TEST(PublisherTest, MessagePublisher)
{
  MessagePublisher publisher(Topic, Addr, Ctrl, PUuid, NUuid, Scope,
    MsgTypeName);
  EXPECT_EQ(publisher.Topic(), Topic);
  EXPECT_EQ(publisher.Addr(),  Addr);
  EXPECT_EQ(publisher.Ctrl(),  Ctrl);
  EXPECT_EQ(publisher.PUuid(), PUuid);
  EXPECT_EQ(publisher.NUuid(), NUuid);
  EXPECT_EQ(publisher.Scope(), Scope);
  EXPECT_EQ(publisher.MsgTypeName(), MsgTypeName);

  MessagePublisher pub2(publisher);
  EXPECT_TRUE(publisher == pub2);
  EXPECT_FALSE(publisher != pub2);

  // Modify the publisher's member variables.
  publisher.SetTopic(NewTopic);
  publisher.SetAddr(NewAddr);
  publisher.SetCtrl(NewCtrl);
  publisher.SetPUuid(NewPUuid);
  publisher.SetNUuid(NewNUuid);
  publisher.SetScope(NewScope);
  publisher.SetMsgTypeName(NewMsgTypeName);

  EXPECT_EQ(publisher.Topic(), NewTopic);
  EXPECT_EQ(publisher.Addr(),  NewAddr);
  EXPECT_EQ(publisher.Ctrl(),  NewCtrl);
  EXPECT_EQ(publisher.PUuid(), NewPUuid);
  EXPECT_EQ(publisher.NUuid(), NewNUuid);
  EXPECT_EQ(publisher.Scope(), NewScope);
  EXPECT_EQ(publisher.MsgTypeName(), NewMsgTypeName);
}

//////////////////////////////////////////////////
/// \brief Check the MessagePublisher Pack()/Unpack().
TEST(PublisherTest, MessagePublisherIO)
{
  // Try to pack an empty publisher.
  MessagePublisher emptyPublisher;
  std::vector<char> buffer(emptyPublisher.MsgLength());
  EXPECT_EQ(emptyPublisher.Pack(&buffer[0]), 0u);

  // Pack a Publisher.
  MessagePublisher publisher(Topic, Addr, Ctrl, PUuid, NUuid, Scope,
    MsgTypeName);

  buffer.resize(publisher.MsgLength());
  size_t bytes = publisher.Pack(&buffer[0]);
  EXPECT_EQ(bytes, publisher.MsgLength());

  // Unpack the Publisher.
  MessagePublisher otherPublisher;
  otherPublisher.Unpack(&buffer[0]);

  // Check that after Pack() and Unpack() the Publisher remains the same.
  EXPECT_EQ(publisher.Topic(), otherPublisher.Topic());
  EXPECT_EQ(publisher.Addr(),  otherPublisher.Addr());
  EXPECT_EQ(publisher.Ctrl(),  otherPublisher.Ctrl());
  EXPECT_EQ(publisher.PUuid(), otherPublisher.PUuid());
  EXPECT_EQ(publisher.NUuid(), otherPublisher.NUuid());
  EXPECT_EQ(publisher.Scope(), otherPublisher.Scope());
  EXPECT_EQ(publisher.MsgTypeName(), otherPublisher.MsgTypeName());

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}

//////////////////////////////////////////////////
/// \brief Check the ServicePublisher accessors.
TEST(PublisherTest, ServicePublisher)
{
  ServicePublisher publisher(Topic, Addr, SocketId, PUuid, NUuid, Scope,
    ReqTypeName, RepTypeName);
  EXPECT_EQ(publisher.Topic(), Topic);
  EXPECT_EQ(publisher.Addr(),  Addr);
  EXPECT_EQ(publisher.SocketId(), SocketId);
  EXPECT_EQ(publisher.PUuid(), PUuid);
  EXPECT_EQ(publisher.NUuid(), NUuid);
  EXPECT_EQ(publisher.Scope(), Scope);
  EXPECT_EQ(publisher.ReqTypeName(), ReqTypeName);
  EXPECT_EQ(publisher.RepTypeName(), RepTypeName);

  ServicePublisher pub2(publisher);
  EXPECT_TRUE(publisher == pub2);
  EXPECT_FALSE(publisher != pub2);

  // Modify the publisher's member variables.
  publisher.SetTopic(NewTopic);
  publisher.SetAddr(NewAddr);
  publisher.SetSocketId(NewSocketId);
  publisher.SetPUuid(NewPUuid);
  publisher.SetNUuid(NewNUuid);
  publisher.SetScope(NewScope);
  publisher.SetReqTypeName(NewReqTypeName);
  publisher.SetRepTypeName(NewRepTypeName);

  EXPECT_EQ(publisher.Topic(), NewTopic);
  EXPECT_EQ(publisher.Addr(),  NewAddr);
  EXPECT_EQ(publisher.SocketId(),  NewSocketId);
  EXPECT_EQ(publisher.PUuid(), NewPUuid);
  EXPECT_EQ(publisher.NUuid(), NewNUuid);
  EXPECT_EQ(publisher.Scope(), NewScope);
  EXPECT_EQ(publisher.ReqTypeName(), NewReqTypeName);
  EXPECT_EQ(publisher.RepTypeName(), NewRepTypeName);
}

//////////////////////////////////////////////////
/// \brief Check the ServicePublisher Pack()/Unpack().
TEST(PublisherTest, ServicePublisherIO)
{
  // Try to pack an empty publisher.
  ServicePublisher emptyPublisher;
  std::vector<char> buffer(emptyPublisher.MsgLength());
  EXPECT_EQ(emptyPublisher.Pack(&buffer[0]), 0u);

  // Pack a Publisher.
  ServicePublisher publisher(Topic, Addr, SocketId, PUuid, NUuid, Scope,
    ReqTypeName, RepTypeName);

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
  EXPECT_EQ(publisher.Scope(), otherPublisher.Scope());
  EXPECT_EQ(publisher.ReqTypeName(), otherPublisher.ReqTypeName());
  EXPECT_EQ(publisher.RepTypeName(), otherPublisher.RepTypeName());

  // Try to pack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Pack(nullptr), 0u);

  // Try to unpack a header passing a NULL buffer.
  EXPECT_EQ(otherPublisher.Unpack(nullptr), 0u);
}
