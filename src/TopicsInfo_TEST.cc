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
TEST(PacketTest, BasicTopicsInfoAPI)
{
  transport::TopicsInfo topics;
  std::string topic = "foo";
  std::string addr1 = "tcp://10.0.0.1:6000";
  std::string ctrl1 = "tcp://10.0.0.1:6001";
  std::string addr2 = "tcp://10.0.0.1:6002";
  std::string ctrl2 = "tcp://10.0.0.1:6003";
  std::string addr3 = "tcp://10.0.0.1:6004";
  std::string ctrl3 = "tcp://10.0.0.1:6005";
  std::string addr4 = "tcp://10.0.0.1:6006";
  std::string ctrl4 = "tcp://10.0.0.1:6007";
  std::string uuid1 = "uuid1";
  std::string uuid2 = "uuid2";
  transport::Addresses_M m;
  transport::ReqCallback reqCb;
  transport::RepCallback repCb;

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
  topics.AddAdvAddress(topic, addr1, ctrl1, uuid1);
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Only contains information about one process.
  EXPECT_EQ(m.size(), 1);
  ASSERT_NE(m.find(uuid1), m.end());
  auto v = m[uuid1];
  // Only contains information about one node.
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0).addr, addr1);
  EXPECT_EQ(v.at(0).ctrl, ctrl1);

  // Insert one node address on the same process.
  topics.AddAdvAddress(topic, addr2, ctrl2, uuid1);
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Only contains information about one process.
  EXPECT_EQ(m.size(), 1);
  ASSERT_NE(m.find(uuid1), m.end());
  v = m[uuid1];
  // Only contains information about one node.
  ASSERT_EQ(v.size(), 2);
  EXPECT_EQ(v.at(0).addr, addr1);
  EXPECT_EQ(v.at(0).ctrl, ctrl1);
  EXPECT_EQ(v.at(1).addr, addr2);
  EXPECT_EQ(v.at(1).ctrl, ctrl2);

  // Insert one node address on a second process.
  topics.AddAdvAddress(topic, addr3, ctrl3, uuid2);
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr3));
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Contains information about two processes.
  EXPECT_EQ(m.size(), 2);
  EXPECT_NE(m.find(uuid1), m.end());
  ASSERT_NE(m.find(uuid2), m.end());
  v = m[uuid2];
  // Only contains information about one node.
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0).addr, addr3);
  EXPECT_EQ(v.at(0).ctrl, ctrl3);

  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Insert another node on process2.
  topics.AddAdvAddress(topic, addr4, ctrl4, uuid2);
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
  EXPECT_NE(m.find(uuid1), m.end());
  ASSERT_NE(m.find(uuid2), m.end());
  v = m[uuid2];
  // Only contains information about node4.
  ASSERT_EQ(v.size(), 1);
  EXPECT_NE(v.at(0).addr, addr4);
  EXPECT_NE(v.at(0).ctrl, ctrl4);

  // Remove the remaining address of process2.
  topics.DelAdvAddress(topic, addr3, "");
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr1));
  EXPECT_TRUE(topics.HasAdvAddress(topic, addr2));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr3));
  EXPECT_FALSE(topics.HasAdvAddress(topic, addr4));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, m));
  // Contains information about 1 process.
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.find(uuid2), m.end());

  // Remove all the addresses of process1.
  topics.DelAdvAddress(topic, "", uuid1);
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
