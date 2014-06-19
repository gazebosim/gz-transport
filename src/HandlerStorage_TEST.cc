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

#include <robot_msgs/int.pb.h>
#include <robot_msgs/stringmsg.pb.h>
#include <robot_msgs/vector2d.pb.h>
#include <map>
#include <string>
#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/TransportTypes.hh"
#include "gtest/gtest.h"

using namespace ignition;

// Global variables used for multiple tests.
std::string topic   = "foo";
std::string reqData = "Walter White";
std::string nUuid1  = "node-UUID-1";
std::string nUuid2  = "node-UUID-2";
std::string hUuid   = "handler-UUID";
bool cbExecuted = false;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  cbExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Callback providing a service call.
void cb1(const std::string &_topic, const robot_msgs::StringMsg &_req,
  robot_msgs::Int &_rep, bool &_result)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_req.data(), reqData);
  _rep.set_data(reqData.size());
  _result = true;

  cbExecuted = true;
}

//////////////////////////////////////////////////
/// \brief Check all the methods of the RepStorage helper class.
TEST(RepStorageTest, RepStorageAPI)
{
  transport::IRepHandlerPtr handler;
  std::map<std::string, std::map<std::string, transport::IRepHandlerPtr>> m;
  transport::HandlerStorage<transport::IRepHandler> reps;
  robot_msgs::Int rep1Msg;
  bool result;

  robot_msgs::StringMsg reqMsg;
  reqMsg.set_data(reqData);

  // Check some operations when there is no data stored.
  EXPECT_FALSE(reps.GetHandlers(topic, m));
  EXPECT_FALSE(reps.GetHandler(topic, handler));
  EXPECT_FALSE(reps.GetHandler(topic, nUuid1, hUuid, handler));
  EXPECT_FALSE(reps.HasHandlersForTopic(topic));
  EXPECT_FALSE(reps.RemoveHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid1));

  // Create a REP handler.
  std::shared_ptr<transport::RepHandler<robot_msgs::StringMsg, robot_msgs::Int>>
    rep1HandlerPtr(new transport::RepHandler
      <robot_msgs::StringMsg, robot_msgs::Int>());

  rep1HandlerPtr->SetCallback(cb1);

  // Insert the handler and check operations.
  reps.AddHandler(topic, nUuid1, rep1HandlerPtr);
  EXPECT_TRUE(reps.HasHandlersForTopic(topic));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid2));
  EXPECT_TRUE(reps.GetHandler(topic, handler));
  std::string handlerUuid = handler->GetHandlerUuid();
  EXPECT_EQ(handlerUuid, rep1HandlerPtr->GetHandlerUuid());
  EXPECT_TRUE(reps.GetHandler(topic, nUuid1, handlerUuid, handler));
  EXPECT_FALSE(reps.GetHandler(topic, "wrongNodeUuid", handlerUuid, handler));
  EXPECT_FALSE(reps.GetHandler(topic, nUuid1, "wrongHandlerUuid", handler));
  EXPECT_TRUE(reps.GetHandlers(topic, m));
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, nUuid1);

  reset();

  // Check the handler operations.
  handler = m[nUuid1].begin()->second;
  handler->RunLocalCallback(topic, reqMsg, rep1Msg, result);
  EXPECT_TRUE(cbExecuted);
  EXPECT_EQ(rep1Msg.data(), reqData.size());
  EXPECT_TRUE(result);

  reset();

  std::string reqSerialized;
  std::string repSerialized;
  reqMsg.SerializeToString(&reqSerialized);
  handler->RunCallback(topic, reqSerialized, repSerialized, result);
  EXPECT_TRUE(cbExecuted);
  EXPECT_TRUE(result);
  rep1Msg.ParseFromString(repSerialized);
  EXPECT_EQ(rep1Msg.data(), reqData.size());

  // Create another REP handler without a callback for node1.
  std::shared_ptr<transport::RepHandler<robot_msgs::Vector2d, robot_msgs::Int>>
    rep2HandlerPtr(new transport::RepHandler
      <robot_msgs::Vector2d, robot_msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep2HandlerPtr);

  // Create a REP handler without a callback for node2.
  std::shared_ptr<transport::RepHandler<robot_msgs::Vector2d, robot_msgs::Int>>
    rep3HandlerPtr(new transport::RepHandler
      <robot_msgs::Vector2d, robot_msgs::Int>());

  // Insert the handler and check operations.
  reps.AddHandler(topic, nUuid2, rep3HandlerPtr);
  EXPECT_TRUE(reps.HasHandlersForTopic(topic));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid2));
  EXPECT_TRUE(reps.GetHandler(topic, handler));
  handlerUuid = rep3HandlerPtr->GetHandlerUuid();
  EXPECT_TRUE(reps.GetHandler(topic, nUuid2, handlerUuid, handler));
  EXPECT_EQ(handler->GetHandlerUuid(), handlerUuid);
  EXPECT_TRUE(reps.GetHandlers(topic, m));
  EXPECT_EQ(m.size(), 2);

  reset();

  // Check the handler operations.
  handler = m[nUuid2].begin()->second;
  handler->RunLocalCallback(topic, reqMsg, rep1Msg, result);
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(result);

  reset();

  handler->RunCallback(topic, reqSerialized, repSerialized, result);
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(result);

  // Remove the last REP handler.
  EXPECT_TRUE(reps.RemoveHandler(topic, nUuid2, handler->GetHandlerUuid()));
  EXPECT_TRUE(reps.HasHandlersForTopic(topic));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid2));
  EXPECT_TRUE(reps.GetHandlers(topic, m));
  EXPECT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, nUuid1);

  reset();

  // Remove all REP handlers for node1.
  EXPECT_TRUE(reps.RemoveHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.GetHandlers(topic, m));
  EXPECT_FALSE(reps.HasHandlersForTopic(topic));
  EXPECT_FALSE(reps.RemoveHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid1));

  // Insert another handler, remove it, and check that the map is empty.
  std::shared_ptr<transport::RepHandler<robot_msgs::Vector2d, robot_msgs::Int>>
    rep4HandlerPtr(new transport::RepHandler
      <robot_msgs::Vector2d, robot_msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep3HandlerPtr);
  handlerUuid = rep3HandlerPtr->GetHandlerUuid();
  EXPECT_TRUE(reps.RemoveHandler(topic, nUuid1, handlerUuid));
  EXPECT_FALSE(reps.HasHandlersForTopic(topic));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid2));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
