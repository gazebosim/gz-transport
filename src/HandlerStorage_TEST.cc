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

#include "msgs/int.pb.h"
#include "msgs/vector3d.pb.h"

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
void cb1(const transport::msgs::Vector3d &_req, transport::msgs::Int &_rep,
  bool &_result)
{
  EXPECT_DOUBLE_EQ(_req.x(), 1.0);
  EXPECT_DOUBLE_EQ(_req.y(), 2.0);
  EXPECT_DOUBLE_EQ(_req.z(), 3.0);
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
  std::string reqType = reqMsg.GetTypeName();
  std::string rep1Type = rep1Msg.GetTypeName();

  reqMsg.set_x(1.0);
  reqMsg.set_y(2.0);
  reqMsg.set_z(3.0);

  // Check some operations when there is no data stored.
  EXPECT_FALSE(reps.Handlers(topic, m));
  EXPECT_FALSE(reps.FirstHandler(topic, reqType, rep1Type, handler));
  EXPECT_FALSE(reps.Handler(topic, nUuid1, hUuid, handler));
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
  EXPECT_TRUE(reps.FirstHandler(topic, reqType, rep1Type, handler));
  ASSERT_TRUE(handler != NULL);
  std::string handlerUuid = handler->HandlerUuid();
  EXPECT_EQ(handlerUuid, rep1HandlerPtr->HandlerUuid());
  EXPECT_TRUE(reps.Handler(topic, nUuid1, handlerUuid, handler));
  EXPECT_FALSE(reps.Handler(topic, "wrongNodeUuid", handlerUuid, handler));
  EXPECT_FALSE(reps.Handler(topic, nUuid1, "wrongHandlerUuid", handler));
  EXPECT_TRUE(reps.Handlers(topic, m));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.begin()->first, nUuid1);

  reset();

  // Check the handler operations.
  handler = m[nUuid1].begin()->second;
  handler->RunLocalCallback(reqMsg, rep1Msg, result);
  EXPECT_TRUE(cbExecuted);
  EXPECT_EQ(rep1Msg.data(), intResult);
  EXPECT_TRUE(result);

  reset();

  std::string reqSerialized;
  std::string repSerialized;
  EXPECT_TRUE(reqMsg.SerializeToString(&reqSerialized));
  handler->RunCallback(reqSerialized, repSerialized, result);
  EXPECT_TRUE(cbExecuted);
  EXPECT_TRUE(result);
  EXPECT_TRUE(rep1Msg.ParseFromString(repSerialized));
  EXPECT_EQ(rep1Msg.data(), intResult);

  // Create another REP handler without a callback for node1.
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep2HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep2HandlerPtr);

  // Create another REP handler without a callback for node1.
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep5HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep5HandlerPtr);
  EXPECT_TRUE(reps.RemoveHandler(topic, nUuid1, rep5HandlerPtr->HandlerUuid()));

  // Create a REP handler without a callback for node2.
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep3HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

  // Insert the handler and check operations.
  reps.AddHandler(topic, nUuid2, rep3HandlerPtr);
  EXPECT_TRUE(reps.HasHandlersForTopic(topic));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid2));
  EXPECT_TRUE(reps.FirstHandler(topic, reqType, rep1Type, handler));
  handlerUuid = rep3HandlerPtr->HandlerUuid();
  EXPECT_TRUE(reps.Handler(topic, nUuid2, handlerUuid, handler));
  EXPECT_EQ(handler->HandlerUuid(), handlerUuid);
  EXPECT_TRUE(reps.Handlers(topic, m));
  EXPECT_EQ(m.size(), 2u);

  reset();

  // Check the handler operations.
  handler = m[nUuid2].begin()->second;
  handler->RunLocalCallback(reqMsg, rep1Msg, result);
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(result);

  reset();

  handler->RunCallback(reqSerialized, repSerialized, result);
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(result);

  // Remove the last REP handler.
  EXPECT_TRUE(reps.RemoveHandler(topic, nUuid2, handler->HandlerUuid()));
  EXPECT_TRUE(reps.HasHandlersForTopic(topic));
  EXPECT_TRUE(reps.HasHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid2));
  EXPECT_TRUE(reps.Handlers(topic, m));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.begin()->first, nUuid1);

  reset();

  // Remove all REP handlers for node1.
  EXPECT_TRUE(reps.RemoveHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.Handlers(topic, m));
  EXPECT_FALSE(reps.HasHandlersForTopic(topic));
  EXPECT_FALSE(reps.RemoveHandlersForNode(topic, nUuid1));
  EXPECT_FALSE(reps.HasHandlersForNode(topic, nUuid1));

  // Insert another handler, remove it, and check that the map is empty.
  std::shared_ptr<transport::RepHandler<transport::msgs::Int,
    transport::msgs::Int>> rep4HandlerPtr(new transport::RepHandler
      <transport::msgs::Int, transport::msgs::Int>());

  // Insert the handler.
  reps.AddHandler(topic, nUuid1, rep3HandlerPtr);
  handlerUuid = rep3HandlerPtr->HandlerUuid();
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
  std::string handlerUuid = sub1HandlerPtr->HandlerUuid();
  EXPECT_TRUE(subs.Handler(topic, nUuid1, handlerUuid, h));
  EXPECT_FALSE(h->RunLocalCallback(msg));

  // Try to retrieve the first callback with an incorrect type.
  transport::ISubscriptionHandlerPtr handler;
  EXPECT_FALSE(subs.FirstHandler(topic, "incorrect type", handler));

  // Now try to retrieve the first callback with the correct type.
  EXPECT_TRUE(subs.FirstHandler(topic, msg.GetTypeName(), handler));

  // Verify the handler.
  EXPECT_EQ(handler->TypeName(), sub1HandlerPtr->TypeName());
  EXPECT_EQ(handler->NodeUuid(), sub1HandlerPtr->NodeUuid());
  EXPECT_EQ(handler->HandlerUuid(), sub1HandlerPtr->HandlerUuid());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
