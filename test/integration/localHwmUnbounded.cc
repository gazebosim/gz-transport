/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_utils.hh"

using namespace gz;
using namespace std::chrono_literals;

//////////////////////////////////////////////////
/// \brief GZ_TRANSPORT_LOCAL_HWM=0 disables the capacity: no drops.
TEST(localHwmUnboundedTest, NoDrops)
{
  ASSERT_EQ(0, transport::localHwm());

  // More messages than the default capacity.
  const int kNumMsgs = 2 * transport::kDefaultLocalHwm;

  // Declared before the node to outlive the subscription.
  std::mutex mutex;
  std::condition_variable cv;
  bool released = false;
  std::vector<int> received;

  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/unbounded");
  ASSERT_TRUE(pub);

  // Block the delivery thread on the first message so the rest queue up.
  std::function<void(const msgs::Int32 &)> cb =
    [&](const msgs::Int32 &_msg)
    {
      std::unique_lock<std::mutex> lk(mutex);
      received.push_back(_msg.data());
      cv.notify_all();
      if (received.size() == 1u)
        cv.wait_for(lk, 30s, [&]{return released;});
    };
  EXPECT_TRUE(node.Subscribe("/unbounded", cb));

  msgs::Int32 msg;
  for (int i = 0; i < kNumMsgs; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pub.Publish(msg));
  }

  std::unique_lock<std::mutex> lk(mutex);
  released = true;
  cv.notify_all();
  ASSERT_TRUE(cv.wait_for(lk, 10s,
    [&]{return received.size() == static_cast<size_t>(kNumMsgs);}));

  for (int i = 0; i < kNumMsgs; ++i)
    EXPECT_EQ(i, received[i]);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  std::string partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);

  // Unlimited local queue.
  gz::utils::setenv("GZ_TRANSPORT_LOCAL_HWM", "0");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
