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

#include <string>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TopicStorage.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check all the methods of the TopicStorage helper class.
TEST(TopicStorageTest, TopicStorageAPI)
{
  std::string topic  = "foo";
  std::string topic2 = "foo2";

  std::string nUuid1 = "node-UUID-1";
  transport::Scope_t scope1 = transport::Scope_t::ALL;
  std::string nUuid2  = "node-UUID-2";
  transport::Scope_t scope2 = transport::Scope_t::PROCESS;
  std::string nUuid3  = "node-UUID-3";
  transport::Scope_t scope3 = transport::Scope_t::HOST;
  std::string nUuid4  = "node-UUID-4";
  transport::Scope_t scope4 = transport::Scope_t::ALL;

  std::string pUuid1 = "process-UUID-1";
  std::string addr1  = "tcp://10.0.0.1:6001";
  std::string pUuid2  = "process-UUID-2";
  std::string addr2  = "tcp://10.0.0.1:6002";

  std::map<std::string, std::vector<transport::Publisher>> m;
  transport::Publisher info;
  transport::TopicStorage<transport::Publisher> test;

  // Check HasTopic.
  EXPECT_FALSE(test.HasTopic(topic));

  // Check HasAnyPublishers.
  EXPECT_FALSE(test.HasAnyPublishers(topic, pUuid1));
  // Check HasPublisher.
  EXPECT_FALSE(test.HasPublisher(addr1));
  // Check Publisher.
  EXPECT_FALSE(test.Publisher(topic, pUuid1, nUuid1, info));
  // Check Publishers.
  EXPECT_FALSE(test.Publishers(topic, m));
  // Try to remove a entry not stored yet.
  EXPECT_FALSE(test.DelPublisherByNode(topic, pUuid2, nUuid4));
  // Try to remove a set of entries within a process not stored.
  EXPECT_FALSE(test.DelPublishersByProc(pUuid1));

  // Insert one node address.
  transport::Publisher publisher1(topic, addr1, pUuid1, nUuid1, scope1);
  EXPECT_TRUE(test.AddPublisher(publisher1));

  // Insert an existing publisher.
  EXPECT_FALSE(test.AddPublisher(publisher1));

  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));

  // Check HasAnyPublishers.
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_TRUE(test.HasPublisher(addr1));
  EXPECT_FALSE(test.HasPublisher(addr2));
  // Check Publisher.
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid1);
  EXPECT_EQ(info.Scope(), scope1);
  EXPECT_FALSE(test.Publisher(topic, "wrong pUuid", nUuid1, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid1, "wrong nUuid", info));
  // Check Publishers.
  EXPECT_TRUE(test.Publishers(topic, m));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(0).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).NUuid(), nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Scope(), scope1);

  // Insert one node address on the same process.
  transport::Publisher publisher2(topic, addr1, pUuid1, nUuid2, scope2);
  EXPECT_TRUE(test.AddPublisher(publisher2));
  EXPECT_FALSE(test.AddPublisher(publisher2));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyPublishers.
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_TRUE(test.HasPublisher(addr1));
  EXPECT_FALSE(test.HasPublisher(addr2));
  EXPECT_FALSE(test.Publisher(topic, "wrong pUuid", nUuid2, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid1, "wrong nUuid", info));
  // Check Publisher.
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid1);
  EXPECT_EQ(info.Scope(), scope1);
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid2);
  EXPECT_EQ(info.Scope(), scope2);
  // Check Publishers.
  EXPECT_TRUE(test.Publishers(topic, m));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(0).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).NUuid(), nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Scope(), scope1);
  EXPECT_EQ(m[pUuid1].at(1).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(1).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(1).NUuid(), nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).Scope(), scope2);

  // Insert a node address on a second process.
  transport::Publisher publisher3(topic, addr2, pUuid2, nUuid3, scope3);
  EXPECT_TRUE(test.AddPublisher(publisher3));
  EXPECT_FALSE(test.AddPublisher(publisher3));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyPublishers.
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_TRUE(test.HasPublisher(addr1));
  EXPECT_TRUE(test.HasPublisher(addr2));
  // Check Publisher.
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid1);
  EXPECT_EQ(info.Scope(), scope1);
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid2);
  EXPECT_EQ(info.Scope(), scope2);
  EXPECT_TRUE(test.Publisher(topic, pUuid2, nUuid3, info));
  EXPECT_EQ(info.Addr(), addr2);
  EXPECT_EQ(info.PUuid(), pUuid2);
  EXPECT_EQ(info.NUuid(), nUuid3);
  EXPECT_EQ(info.Scope(), scope3);
  EXPECT_FALSE(test.Publisher(topic, "wrong pUuid", nUuid3, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid2, "wrong nUuid", info));

  // Check Publishers.
  EXPECT_TRUE(test.Publishers(topic, m));
  EXPECT_EQ(m.size(), 2u);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(0).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).NUuid(), nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Scope(), scope1);
  EXPECT_EQ(m[pUuid1].at(1).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(1).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(1).NUuid(), nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).Scope(), scope2);
  EXPECT_EQ(m[pUuid2].at(0).Addr(), addr2);
  EXPECT_EQ(m[pUuid2].at(0).PUuid(), pUuid2);
  EXPECT_EQ(m[pUuid2].at(0).NUuid(), nUuid3);
  EXPECT_EQ(m[pUuid2].at(0).Scope(), scope3);

  // Insert another node on process2.
  transport::Publisher publisher4(topic, addr2, pUuid2, nUuid4, scope4);
  EXPECT_TRUE(test.AddPublisher(publisher4));
  EXPECT_FALSE(test.AddPublisher(publisher4));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyPublishers.
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_TRUE(test.HasPublisher(addr1));
  EXPECT_TRUE(test.HasPublisher(addr2));
  // Check Publisher.
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid1);
  EXPECT_EQ(info.Scope(), scope1);
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid2);
  EXPECT_EQ(info.Scope(), scope2);
  EXPECT_TRUE(test.Publisher(topic, pUuid2, nUuid3, info));
  EXPECT_EQ(info.Addr(), addr2);
  EXPECT_EQ(info.PUuid(), pUuid2);
  EXPECT_EQ(info.NUuid(), nUuid3);
  EXPECT_EQ(info.Scope(), scope3);
  EXPECT_TRUE(test.Publisher(topic, pUuid2, nUuid4, info));
  EXPECT_EQ(info.Addr(), addr2);
  EXPECT_EQ(info.PUuid(), pUuid2);
  EXPECT_EQ(info.NUuid(), nUuid4);
  EXPECT_EQ(info.Scope(), scope4);
  EXPECT_FALSE(test.Publisher(topic, "wrong pUuid", nUuid4, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid2, "wrong nUuid", info));
  // Check Publishers.
  EXPECT_TRUE(test.Publishers(topic, m));
  EXPECT_EQ(m.size(), 2u);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(0).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).NUuid(), nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Scope(), scope1);
  EXPECT_EQ(m[pUuid1].at(1).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(1).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(1).NUuid(), nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).Scope(), scope2);
  EXPECT_EQ(m[pUuid2].at(0).Addr(), addr2);
  EXPECT_EQ(m[pUuid2].at(0).PUuid(), pUuid2);
  EXPECT_EQ(m[pUuid2].at(0).NUuid(), nUuid3);
  EXPECT_EQ(m[pUuid2].at(0).Scope(), scope3);
  EXPECT_EQ(m[pUuid2].at(1).Addr(), addr2);
  EXPECT_EQ(m[pUuid2].at(1).PUuid(), pUuid2);
  EXPECT_EQ(m[pUuid2].at(1).NUuid(), nUuid4);
  EXPECT_EQ(m[pUuid2].at(1).Scope(), scope4);

  // Check that Print() does not break anything.
  test.Print();

  // Remove the node4's address advertised for topic.
  EXPECT_TRUE(test.DelPublisherByNode(topic, pUuid2, nUuid4));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyPublishers.
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_TRUE(test.HasPublisher(addr1));
  EXPECT_TRUE(test.HasPublisher(addr2));
  // Check Publisher.
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid1);
  EXPECT_EQ(info.Scope(), scope1);
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid2);
  EXPECT_EQ(info.Scope(), scope2);
  EXPECT_TRUE(test.Publisher(topic, pUuid2, nUuid3, info));
  EXPECT_EQ(info.Addr(), addr2);
  EXPECT_EQ(info.PUuid(), pUuid2);
  EXPECT_EQ(info.NUuid(), nUuid3);
  EXPECT_EQ(info.Scope(), scope3);
  // Check Publishers.
  EXPECT_TRUE(test.Publishers(topic, m));
  EXPECT_EQ(m.size(), 2u);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(0).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).NUuid(), nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Scope(), scope1);
  EXPECT_EQ(m[pUuid1].at(1).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(1).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(1).NUuid(), nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).Scope(), scope2);
  EXPECT_EQ(m[pUuid2].at(0).Addr(), addr2);
  EXPECT_EQ(m[pUuid2].at(0).PUuid(), pUuid2);
  EXPECT_EQ(m[pUuid2].at(0).NUuid(), nUuid3);
  EXPECT_EQ(m[pUuid2].at(0).Scope(), scope3);

  // Remove the node3's address advertised for topic.
  EXPECT_TRUE(test.DelPublisherByNode(topic, pUuid2, nUuid3));
  // Check HasTopic.
  EXPECT_TRUE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyPublishers.
  EXPECT_TRUE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_TRUE(test.HasPublisher(addr1));
  EXPECT_FALSE(test.HasPublisher(addr2));
  // Check Publisher.
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid1);
  EXPECT_EQ(info.Scope(), scope1);
  EXPECT_TRUE(test.Publisher(topic, pUuid1, nUuid2, info));
  EXPECT_EQ(info.Addr(), addr1);
  EXPECT_EQ(info.PUuid(), pUuid1);
  EXPECT_EQ(info.NUuid(), nUuid2);
  EXPECT_EQ(info.Scope(), scope2);
  // Check Publishers.
  EXPECT_TRUE(test.Publishers(topic, m));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.begin()->first, pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(0).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(0).NUuid(), nUuid1);
  EXPECT_EQ(m[pUuid1].at(0).Scope(), scope1);
  EXPECT_EQ(m[pUuid1].at(1).Addr(), addr1);
  EXPECT_EQ(m[pUuid1].at(1).PUuid(), pUuid1);
  EXPECT_EQ(m[pUuid1].at(1).NUuid(), nUuid2);
  EXPECT_EQ(m[pUuid1].at(1).Scope(), scope2);

  // Insert another publisher on topic2.
  transport::Publisher publisher9(topic2, addr2, pUuid2, nUuid2, scope2);
  EXPECT_TRUE(test.AddPublisher(publisher9));

  // Remove all the addresses of process1.
  EXPECT_TRUE(test.DelPublishersByProc(pUuid1));

  EXPECT_TRUE(test.DelPublisherByNode(topic2, pUuid2, nUuid2));

  // Check HasTopic.
  EXPECT_FALSE(test.HasTopic(topic));
  EXPECT_FALSE(test.HasTopic("Unknown topic"));
  // Check HasAnyPublishers.
  EXPECT_FALSE(test.HasAnyPublishers(topic, pUuid1));
  EXPECT_FALSE(test.HasAnyPublishers(topic, pUuid2));
  // Check HasPublishers.
  EXPECT_FALSE(test.HasPublisher(addr1));
  EXPECT_FALSE(test.HasPublisher(addr2));
  // Check Publisher.
  EXPECT_FALSE(test.Publisher(topic, pUuid1, nUuid1, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid1, nUuid2, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid2, nUuid3, info));
  EXPECT_FALSE(test.Publisher(topic, pUuid2, nUuid4, info));
  // Check Publishers.
  EXPECT_FALSE(test.Publishers(topic, m));

  // Insert a topic, remove it, and check that the map is empty.
  transport::Publisher publisher5(topic, addr1, pUuid1, nUuid1, scope1);
  EXPECT_TRUE(test.AddPublisher(publisher5));
  EXPECT_TRUE(test.DelPublisherByNode(topic, pUuid1, nUuid1));
  EXPECT_FALSE(test.HasTopic(topic));

  // Insert some topics, and remove all the topics from a process but keeping
  // the same topics from other proccesses.
  transport::Publisher publisher6(topic, addr1, pUuid1, nUuid1, scope1);
  transport::Publisher publisher7(topic, addr1, pUuid1, nUuid2, scope2);
  transport::Publisher publisher8(topic, addr2, pUuid2, nUuid3, scope3);
  EXPECT_TRUE(test.AddPublisher(publisher6));
  EXPECT_TRUE(test.AddPublisher(publisher7));
  EXPECT_TRUE(test.AddPublisher(publisher8));
  EXPECT_TRUE(test.DelPublishersByProc(pUuid1));
}
