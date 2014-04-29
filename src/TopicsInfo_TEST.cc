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

#include <google/protobuf/message.h>
#include <robot_msgs/stringmsg.pb.h>
#include <memory>
#include <string>
#include "ignition/transport/TopicsInfo.hh"
#include "gtest/gtest.h"

using namespace ignition;

bool callbackExecuted = false;

//////////////////////////////////////////////////
void myCb(const std::string &/*_p1*/, const std::string &/*_p2*/)
{
  callbackExecuted = true;
}

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
void cb4(const std::string &_topic,
         const std::shared_ptr<robot_msgs::StringMsg> &/*_msgPtr*/)
{
  assert(_topic != "");

  callbackExecuted = true;
}

//////////////////////////////////////////////////
TEST(PacketTest, BasicTopicsInfoAPI)
{
  transport::TopicsInfo topics;
  std::string topic = "test_topic";
  std::string address = "tcp://10.0.0.1:6000";
  transport::Topics_L v;
  transport::Callback cb;
  transport::ReqCallback reqCb;
  transport::RepCallback repCb;

  // Check getters with an empty TopicsInfo object
  EXPECT_FALSE(topics.HasTopic(topic));
  EXPECT_FALSE(topics.GetAdvAddresses(topic, v));
  EXPECT_FALSE(topics.HasAdvAddress(topic, address));
  EXPECT_FALSE(topics.Connected(topic));
  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetCallback(topic, cb));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Check getters after inserting a topic in a TopicsInfo object
  topics.AddAdvAddress(topic, address);
  EXPECT_TRUE(topics.HasTopic(topic));
  EXPECT_TRUE(topics.HasAdvAddress(topic, address));
  EXPECT_TRUE(topics.GetAdvAddresses(topic, v));
  EXPECT_EQ(v.at(0), address);
  EXPECT_FALSE(topics.Connected(topic));
  EXPECT_FALSE(topics.Subscribed(topic));
  EXPECT_FALSE(topics.AdvertisedByMe(topic));
  EXPECT_FALSE(topics.Requested(topic));
  EXPECT_FALSE(topics.GetCallback(topic, cb));
  EXPECT_FALSE(topics.GetReqCallback(topic, reqCb));
  EXPECT_FALSE(topics.GetRepCallback(topic, repCb));
  EXPECT_FALSE(topics.PendingReqs(topic));

  // Check that there's only one copy stored of the same address
  topics.AddAdvAddress(topic, address);
  EXPECT_TRUE(topics.GetAdvAddresses(topic, v));
  EXPECT_EQ(v.size(), 1);

  // Check SetConnected
  topics.SetConnected(topic, true);
  EXPECT_TRUE(topics.Connected(topic));

  // Check SetSubscribed
  topics.SetSubscribed(topic, true);
  EXPECT_TRUE(topics.Subscribed(topic));

  // Check SetRequested
  topics.SetRequested(topic, true);
  EXPECT_TRUE(topics.Requested(topic));

  // Check SetAdvertisedByMe
  topics.SetAdvertisedByMe(topic, true);
  EXPECT_TRUE(topics.AdvertisedByMe(topic));

  // Check SetCallback
  topics.SetCallback(topic, myCb);
  //topics.SetCallback(topic, cb4);
  EXPECT_TRUE(topics.GetCallback(topic, cb));
  callbackExecuted = false;
  cb("topic", "data");
  EXPECT_TRUE(callbackExecuted);

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

  // Check the address removal
  topics.RemoveAdvAddress(topic, address);
  EXPECT_FALSE(topics.HasAdvAddress(topic, address));
  EXPECT_FALSE(topics.GetAdvAddresses(topic, v));

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
