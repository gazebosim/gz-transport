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

#include <iostream>
#include <string>
#include "ignition/transport/TopicsInfo.hh"
#include "gtest/gtest.h"

using namespace ignition;

bool callbackExecuted = false;

//////////////////////////////////////////////////
void myReqCb(const std::string &/*_p1*/, int /*p2*/, const std::string &/*p3*/)
{
  callbackExecuted = true;
}

//////////////////////////////////////////////////
int myRepCb(const std::string &/*p1*/, const std::string &/*p2*/,
            std::string &/*p3*/)
{
  callbackExecuted = true;
  return 0;
}

//////////////////////////////////////////////////
/// \brief Check all the methods of the TopicsInfo helper class.
TEST(TopicsInfoTest, BasicTopicsInfoAPI)
{
  transport::TopicsInfo topics;
  std::string topic  = "foo";
  std::string addr1  = "tcp://10.0.0.1:6000";
  std::string ctrl1  = "tcp://10.0.0.1:60011";
  std::string nUuid1 = "node-UUID-1";
  transport::Scope scope1 = transport::Scope::All;
  std::string addr2  = "tcp://10.0.0.1:6002";
  std::string ctrl2  = "tcp://10.0.0.1:60033";
  std::string nUuid2  = "node-UUID-2";
  transport::Scope scope2 = transport::Scope::Process;
  std::string addr3  = "tcp://10.0.0.1:6004";
  std::string ctrl3  = "tcp://10.0.0.1:60055";
  std::string nUuid3  = "node-UUID-3";
  transport::Scope scope3 = transport::Scope::Host;
  std::string addr4  = "tcp://10.0.0.1:6006";
  std::string ctrl4  = "tcp://10.0.0.1:60077";
  std::string nUuid4  = "node-UUID-4";
  transport::Scope scope4 = transport::Scope::All;
  std::string pUuid1 = "process-UUID-1";
  std::string pUuid2  = "process-UUID-2";

  transport::Addresses_M m;
  transport::ReqCallback reqCb;
  transport::RepCallback repCb;
  transport::Address_t info;

  // Check getters with an empty TopicsInfo object
  EXPECT_FALSE(topics.HasTopic(topic));
  EXPECT_FALSE(topics.GetAdvAddresses(topic, m));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr1));
  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Insert one node address.
  topics.AddAdvAddress(topic, addr1, ctrl1, pUuid1, nUuid1, scope1);
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Only contains information about one process.
  EXPECT_EQ(m.size(), 1);
  ASSERT_NE(m.find(pUuid1), m.end());
  auto v = m[pUuid1];
  // Only contains information about one node.
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0).addr, addr1);
  EXPECT_EQ(v.at(0).ctrl, ctrl1);
  EXPECT_EQ(v.at(0).nUuid, nUuid1);
  EXPECT_EQ(v.at(0).scope, scope1);
  // Check GetInfo
  topics.GetInfo(topic, nUuid1, info);
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.ctrl, ctrl1);
  EXPECT_EQ(info.nUuid, nUuid1);
  EXPECT_EQ(info.addr, addr1);
  EXPECT_EQ(info.scope, scope1);

  // Insert one node address on the same process.
  topics.AddAdvAddress(topic, addr2, ctrl2, pUuid1, nUuid2, scope2);
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Only contains information about one process.
  EXPECT_EQ(m.size(), 1);
  ASSERT_NE(m.find(pUuid1), m.end());
  v = m[pUuid1];
  // Only contains information about one node.
  ASSERT_EQ(v.size(), 2);
  EXPECT_EQ(v.at(0).addr, addr1);
  EXPECT_EQ(v.at(0).ctrl, ctrl1);
  EXPECT_EQ(v.at(0).nUuid, nUuid1);
  EXPECT_EQ(v.at(0).scope, scope1);
  EXPECT_EQ(v.at(1).addr, addr2);
  EXPECT_EQ(v.at(1).ctrl, ctrl2);
  EXPECT_EQ(v.at(1).nUuid, nUuid2);
  EXPECT_EQ(v.at(1).scope, scope2);
  // Check GetInfo
  topics.GetInfo(topic, nUuid2, info);
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.ctrl, ctrl2);
  EXPECT_EQ(info.nUuid, nUuid2);
  EXPECT_EQ(info.addr, addr2);
  EXPECT_EQ(info.scope, scope2);

  // Insert one node address on a second process.
  topics.AddAdvAddress(topic, addr3, ctrl3, pUuid2, nUuid3, scope3);
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr3));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Contains information about two processes.
  EXPECT_EQ(m.size(), 2);
  EXPECT_NE(m.find(pUuid1), m.end());
  ASSERT_NE(m.find(pUuid2), m.end());
  v = m[pUuid2];
  // Only contains information about one node.
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0).addr, addr3);
  EXPECT_EQ(v.at(0).ctrl, ctrl3);
  EXPECT_EQ(v.at(0).nUuid, nUuid3);
  EXPECT_EQ(v.at(0).scope, scope3);
  // Check GetInfo
  topics.GetInfo(topic, nUuid3, info);
  EXPECT_EQ(info.addr, addr3);
  EXPECT_EQ(info.ctrl, ctrl3);
  EXPECT_EQ(info.nUuid, nUuid3);
  EXPECT_EQ(info.addr, addr3);
  EXPECT_EQ(info.scope, scope3);

  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Insert another node on process2.
  topics.AddAdvAddress(topic, addr4, ctrl4, pUuid2, nUuid4, scope4);
  // Check GetInfo
  topics.GetInfo(topic, nUuid4, info);
  EXPECT_EQ(info.addr, addr4);
  EXPECT_EQ(info.ctrl, ctrl4);
  EXPECT_EQ(info.nUuid, nUuid4);
  EXPECT_EQ(info.addr, addr4);
  EXPECT_EQ(info.scope, scope4);

  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr3));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr4));

  // Remove one address of process2.
  topics.DelAdvAddress(topic, addr4, "");
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr3));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr4));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Contains information about two processes.
  EXPECT_EQ(m.size(), 2);
  EXPECT_NE(m.find(pUuid1), m.end());
  ASSERT_NE(m.find(pUuid2), m.end());
  v = m[pUuid2];
  // Only contains information about node3.
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0).addr, addr3);
  EXPECT_EQ(v.at(0).ctrl, ctrl3);
  EXPECT_EQ(v.at(0).nUuid, nUuid3);
  EXPECT_EQ(v.at(0).scope, scope3);

  // Remove the remaining address of process2.
  topics.DelAdvAddress(topic, addr3, "");
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr3));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr4));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Contains information about 1 process.
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.find(pUuid2), m.end());

  // Remove all the addresses of process1.
  topics.DelAdvAddress(topic, "", pUuid1);
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr1));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr2));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr3));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr4));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  EXPECT_TRUE(m.empty());

  // Check SetRequested
  topics.SetRequested(topic, true);
  EXPECT_TRUE(topics.Requested(topic));

  // Check SetAdvertisedByMe
  topics.SetAdvertisedByMe(topic, true);
  EXPECT_TRUE(topics.AdvertisedByMe(topic));

  // Check SetReqCallback
  topics.SetReqCallback(topic, myReqCb);
  EXPECT_TRUE(topics.GetReqCallback(topic, reqCb));
  callbackExecuted = false;
  reqCb("topic", 0, "answer");
  EXPECT_TRUE(callbackExecuted);

  // Check SetRepCallback
  topics.SetRepCallback(topic, myRepCb);
  EXPECT_TRUE(topics.GetRepCallback(topic, repCb));
  callbackExecuted = false;
  std::string result;
  EXPECT_EQ(repCb("topic", "ReqParams", result), 0);
  EXPECT_TRUE(callbackExecuted);

  // Check the addition of asynchronous service call requests
  std::string req1 = "paramsReq1";
  std::string req2 = "paramsReq2";
  EXPECT_FALSE(topics.DelReq(topic, req1));
  for (auto topicInfo : topics.GetTopicsInfo())
    EXPECT_FALSE(topics.PendingReqs(topicInfo.first));

  topics.AddReq(topic, req1);
  EXPECT_TRUE(topics.PendingReqs(topic));

  topics.AddReq(topic, req2);
  EXPECT_TRUE(topics.PendingReqs(topic));

  EXPECT_TRUE(topics.DelReq(topic, req2));
  EXPECT_TRUE(topics.PendingReqs(topic));

  EXPECT_TRUE(topics.DelReq(topic, req1));
  EXPECT_FALSE(topics.PendingReqs(topic));

  EXPECT_FALSE(topics.DelReq(topic, req1));
  EXPECT_FALSE(topics.PendingReqs(topic));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
