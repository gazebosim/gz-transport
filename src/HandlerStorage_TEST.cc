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

#include <map>
#include <string>
#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TransportTypes.hh"
#include "gtest/gtest.h"
#include "int.pb.h"
#include "vector3d.pb.h"

using namespace ignition;

// Global variables used for multiple tests.
std::string topic   = "foo";
std::string nUuid1  = "node-UUID-1";
std::string nUuid2  = "node-UUID-2";
std::string hUuid   = "handler-UUID";
int intResult       = 4;
bool cbExecuted = false;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  cbExecuted = false;
}

//////////////////////////////////////////////////
/// \brief Callback providing a service call.
void cb1(const std::string &_topic, const transport::msgs::Vector3d &_req,
  transport::msgs::Int &_rep, bool &_result)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_FLOAT_EQ(_req.x(), 1.0);
  EXPECT_FLOAT_EQ(_req.y(), 2.0);
  EXPECT_FLOAT_EQ(_req.z(), 3.0);
  _rep.set_data(intResult);
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
  transport::msgs::Int rep1Msg;
  bool result;

  transport::msgs::Vector3d reqMsg;
  reqMsg.set_x(1.0);
  reqMsg.set_y(2.0);
  reqMsg.set_z(3.0);

  // Check some operations when there is no data stored.
  EXPECT_FALSE(reps.GetHandlers(topic, m));
  EXPECT_FALSE(reps.GetHandler(topic, handler));
  EXPECT_FALSE(reps.GetHandler(topic, nUuid1, hUuid, handler));
  EXPECT_FALSE(reps.HasHandlersForTopic(topic));
  EXPECT_FALSE(reps.RemoveHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid1));

  // Create a REP handler.
  std::shared_ptr<transport::RepHandler<transport::msgs::Vector3d,
    transport::msgs::Int>> rep1HandlerPtr(new transport::RepHandler<
      transport::msgs::Vector3d, transport::msgs::Int>());

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
  EXPECT_EQ(rep1Msg.data(), intResult);
  EXPECT_TRUE(result);

  reset();

  std::string reqSerialized;
  std::string repSerialized;
  reqMsg.SerializeToString(&reqSerialized);
  handler->RunCallback(topic, reqSerialized, repSerialized, result);
  EXPECT_TRUE(cbExecuted);
  EXPECT_TRUE(result);
  rep1Msg.ParseFromString(repSerialized);
  EXPECT_EQ(rep1Msg.data(), intResult);

  // Create another REP handler without a callback for node1.
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep2HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep2HandlerPtr);

  // Create a REP handler without a callback for node2.
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep3HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

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
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep4HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep3HandlerPtr);
  handlerUuid = rep3HandlerPtr->GetHandlerUuid();
  EXPECT_TRUE(reps.RemoveHandler(topic, nUuid1, handlerUuid));
  EXPECT_FALSE(reps.HasHandlersForTopic(topic));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid2));
}

//////////////////////////////////////////////////
/// \brief Check that nothing breaks if we add subscription handlers without
/// registering a callback, and then, we try to execute the callback.
TEST(RepStorageTest, SubStorageNoCallbacks)
{
  transport::ISubscriptionHandlerPtr handler;
  std::map<std::string, std::map<std::string,
    transport::ISubscriptionHandlerPtr>> m;
  transport::HandlerStorage<transport::ISubscriptionHandler> subs;
  transport::msgs::Int msg;
  msg.set_data(5);

  // Create a Subscription handler.
  std::shared_ptr<transport::SubscriptionHandler<transport::msgs::Int>>
    sub1HandlerPtr(new transport::SubscriptionHandler
      <transport::msgs::Int>(nUuid1));

  // Insert the handler and check operations.
  subs.AddHandler(topic, nUuid1, sub1HandlerPtr);

  transport::ISubscriptionHandlerPtr h;
  std::string handlerUuid = sub1HandlerPtr->GetHandlerUuid();
  EXPECT_TRUE(subs.GetHandler(topic, nUuid1, handlerUuid, h));
  EXPECT_FALSE(h->RunLocalCallback(topic, msg));
  EXPECT_FALSE(h->RunCallback(topic, "some data"));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
