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
#include "gtest/gtest.h"

#include <gz/msgs/int32.pb.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#include "gz/transport/WaitHelpers.hh"
#include "gz/transport/Node.hh"
#include "test_utils.hh"

#include <gz/utils/Environment.hh>

using namespace gz;

//////////////////////////////////////////////////
/// \brief waitUntil: predicate immediately true returns true near-instantly.
TEST(WaitHelpersTest, WaitUntilImmediatelyTrue)
{
  auto t1 = std::chrono::steady_clock::now();
  EXPECT_TRUE(transport::waitUntil([]{ return true; }));
  auto t2 = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  EXPECT_LT(elapsed, 100);
}

//////////////////////////////////////////////////
/// \brief waitUntil: predicate never true returns false after timeout.
TEST(WaitHelpersTest, WaitUntilNeverTrue)
{
  auto t1 = std::chrono::steady_clock::now();
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(100)));
  auto t2 = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  EXPECT_GE(elapsed, 90);
  EXPECT_LT(elapsed, 500);
}

//////////////////////////////////////////////////
/// \brief waitUntil: predicate becomes true mid-wait.
TEST(WaitHelpersTest, WaitUntilBecomesTrueMidWait)
{
  std::atomic<int> callCount{0};
  auto pred = [&]() -> bool {
    return ++callCount >= 5;
  };
  EXPECT_TRUE(transport::waitUntil(pred, std::chrono::milliseconds(5000),
      std::chrono::milliseconds(10)));
  EXPECT_GE(callCount.load(), 5);
}

//////////////////////////////////////////////////
/// \brief waitUntil: zero timeout.
TEST(WaitHelpersTest, WaitUntilZeroTimeout)
{
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(0)));
  EXPECT_TRUE(transport::waitUntil([]{ return true; },
      std::chrono::milliseconds(0)));
}

//////////////////////////////////////////////////
/// \brief waitUntil: elapsed time is close to timeout on failure.
TEST(WaitHelpersTest, WaitUntilElapsedCloseToTimeout)
{
  auto t1 = std::chrono::steady_clock::now();
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(200)));
  auto t2 = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  EXPECT_GE(elapsed, 180);
  EXPECT_LT(elapsed, 1000);
}

//////////////////////////////////////////////////
/// \brief waitForService: service not advertised returns false.
TEST(WaitHelpersTest, WaitForServiceNotAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;
  EXPECT_FALSE(transport::waitForService(node, "/nonexistent",
      std::chrono::milliseconds(200)));
}

//////////////////////////////////////////////////
/// \brief waitForService: service already advertised returns true.
TEST(WaitHelpersTest, WaitForServiceAlreadyAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;

  std::function<bool(const msgs::Int32 &, msgs::Int32 &)> cb =
      [](const msgs::Int32 &_req, msgs::Int32 &_rep) -> bool {
    _rep.set_data(_req.data());
    return true;
  };
  EXPECT_TRUE(node.Advertise("/test_svc", cb));

  EXPECT_TRUE(transport::waitForService(node, "/test_svc",
      std::chrono::milliseconds(2000)));
}

//////////////////////////////////////////////////
/// \brief waitForTopic: topic already advertised returns true.
TEST(WaitHelpersTest, WaitForTopicAlreadyAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/test_topic");
  EXPECT_TRUE(pub);

  EXPECT_TRUE(transport::waitForTopic(node, "/test_topic",
      std::chrono::milliseconds(2000)));
}

//////////////////////////////////////////////////
/// \brief Create a separate thread, block it calling waitForShutdown() and
/// emit a SIGINT signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(WaitHelpersTest, waitForShutdownSIGINT)
{
  std::thread aThread([]{transport::waitForShutdown();});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  aThread.join();
}

//////////////////////////////////////////////////
/// \brief Create a separate thread, block it calling waitForShutdown() and
/// emit a SIGTERM signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(WaitHelpersTest, waitForShutdownSIGTERM)
{
  std::thread aThread([]{transport::waitForShutdown();});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGTERM);
  aThread.join();
}
