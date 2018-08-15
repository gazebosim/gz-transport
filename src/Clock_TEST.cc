/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#include <chrono>
#include <thread>

#include <ignition/msgs.hh>

#include "ignition/transport/Clock.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/test_config.h"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check WallClock functionality.
TEST(ClockTests, WallClock)
{
  const transport::WallClock *clock =
      transport::WallClock::Instance();
  ASSERT_TRUE(clock != nullptr);
  EXPECT_TRUE(clock->IsReady());
  const std::chrono::milliseconds sleepTime(100);
  const std::chrono::nanoseconds startTime = clock->Time();
  std::this_thread::sleep_for(sleepTime);
  const std::chrono::nanoseconds endTime = clock->Time();
  EXPECT_GE(endTime - startTime, sleepTime);
}

//////////////////////////////////////////////////
/// \brief Check NetworkClock functionality.
TEST(ClockTests, NetworkClock)
{
  const std::string clockTopicName{"/clock"};
  transport::NetworkClock sim_clock(
      clockTopicName, transport::NetworkClock::TimeBase::SIM);
  EXPECT_FALSE(sim_clock.IsReady());
  transport::Node node;
  transport::Node::Publisher clock_pub =
      node.Advertise<ignition::msgs::Clock>(clockTopicName);
  const std::chrono::milliseconds sleepTime(100);
  ignition::msgs::Clock clock_msg;
  clock_pub.Publish(clock_msg);
  std::this_thread::sleep_for(sleepTime);
  EXPECT_FALSE(sim_clock.IsReady());
  const std::chrono::seconds expectedSecs(54321);
  const std::chrono::nanoseconds expectedNsecs(12345);
  clock_msg.mutable_sim()->set_sec(expectedSecs.count());
  clock_msg.mutable_sim()->set_nsec(expectedNsecs.count());
  clock_pub.Publish(clock_msg);
  std::this_thread::sleep_for(sleepTime);  // Wait for clock distribution
  EXPECT_TRUE(sim_clock.IsReady());
  EXPECT_EQ(sim_clock.Time(), expectedSecs + expectedNsecs);
  sim_clock.SetTime(expectedSecs + expectedNsecs * 2);
  std::this_thread::sleep_for(sleepTime);  // Wait for clock distribution
  EXPECT_EQ(sim_clock.Time(), expectedSecs + expectedNsecs * 2);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
