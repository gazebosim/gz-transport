/*
 * Copyright (C) 2019 Open Source Robotics Foundation
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
#include <ignition/msgs/stringmsg.pb.h>

#include "gtest/gtest.h"
#include "ignition/transport/CIface.h"
#include "ignition/transport/test_config.h"

static bool cbExecuted;

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const char *_data, const size_t _size, const char *_msgType)
{
  ignition::msgs::StringMsg msg;
  msg.ParseFromArray(_data, _size);
  EXPECT_STREQ("ignition.msgs.StringMsg", _msgType);
  EXPECT_EQ(msg.data(), "HELLO");
  cbExecuted = true;
}

//////////////////////////////////////////////////
TEST(CIfaceTest, PubSub)
{
  cbExecuted = false;
  IgnTransportNode *node = ignTransportNodeCreate();
  EXPECT_NE(nullptr, node);

  const char *topic = "/foo";

  // Subscribe
  ASSERT_EQ(0, ignTransportSubscribe(node, topic, cb));

  // Prepare the message.
  ignition::msgs::StringMsg msg;
  msg.set_data("HELLO");

  // Get the size of the serialized message
  int size = msg.ByteSize();

  // Allocate space for the serialized message
  void *buffer = malloc(size);

  ASSERT_NE(nullptr, buffer);

  // Serialize the message.
  msg.SerializeToArray(buffer, size);

  EXPECT_EQ(0,
    ignTransportPublish(node, topic, buffer, msg.GetTypeName().c_str()));

  EXPECT_TRUE(cbExecuted);

  cbExecuted = false;

  // Unsubscribe
  ASSERT_EQ(0, ignTransportUnsubscribe(node, topic));
  EXPECT_EQ(0,
    ignTransportPublish(node, topic, buffer, msg.GetTypeName().c_str()));
  EXPECT_FALSE(cbExecuted);

  free(buffer);
  ignTransportNodeDestroy(&node);
  EXPECT_EQ(nullptr, node);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  std::string partition = testing::getRandomNumber();

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  // Enable verbose mode.
  // setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
