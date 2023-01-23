/*
 * Copyright (C) 2022 Open Source Robotics Foundation
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
#include <gz/msgs/int32.pb.h>

#include <gtest/gtest.h>

#include <gz/transport/Node.hh>

TEST(CallbackScope, CleanupCorrectly)
{
  gz::transport::Node node;

  auto publisher = node.Advertise<gz::msgs::Int32>("/my_topic");

  gz::msgs::Int32 msg;

  {
    auto msg2 = std::make_unique<gz::msgs::Int32>();

    std::function<void(const gz::msgs::Int32&)> callback =
      [&msg2](const gz::msgs::Int32& msg) {

        if (nullptr == msg2)
        {
          FAIL();
        }

        (void) msg;
        return;
      };

    node.Subscribe("/my_topic", callback);
    publisher.Publish(msg);
    publisher.Publish(msg);
    node.Unsubscribe("/my_topic");

    // Clear msg2
    msg2.reset();
  }
}
