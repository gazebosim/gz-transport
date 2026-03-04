/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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
#include <thread>

#include "gz/transport/Helpers.hh"
#include "gz/transport/Node.hh"
#include "test_utils.hh"

#include <gz/utils/Environment.hh>

using namespace gz;

//////////////////////////////////////////////////
/// \brief Check the env() function.
TEST(HelpersTest, env)
{
  // Create a random string.
  std::string name = testing::getRandomNumber();

  // Check that an unknown environment variable returns false.
  std::string value;
  EXPECT_FALSE(transport::env(name, value));

  // Create a random environment variable and give it its name as value.
  ASSERT_TRUE(gz::utils::setenv(name, name));

  // Check that we find the environment variable and the value is correct.
  EXPECT_TRUE(transport::env(name, value));
  EXPECT_EQ(name, value);
}

/////////////////////////////////////////////////
TEST(HelpersTest, SplitNoDelimiterPresent)
{
  char delim = ':';
  std::string orig = "Hello World!";
  std::vector<std::string> pieces = transport::split(orig, delim);
  ASSERT_LT(0u, pieces.size());
  EXPECT_EQ(1u, pieces.size());
  EXPECT_EQ(orig, pieces[0]);
}

/////////////////////////////////////////////////
TEST(HelpersTest, SplitOneDelimiterInMiddle)
{
  char delim = ' ';
  std::string orig = "Hello World!";
  std::vector<std::string> pieces = transport::split(orig, delim);
  ASSERT_LT(1u, pieces.size());
  EXPECT_EQ(2u, pieces.size());
  EXPECT_EQ("Hello", pieces[0]);
  EXPECT_EQ("World!", pieces[1]);
}

/////////////////////////////////////////////////
TEST(HelpersTest, SplitOneDelimiterAtBeginning)
{
  char delim = ':';
  std::string orig = ":Hello World!";
  std::vector<std::string> pieces = transport::split(orig, delim);
  ASSERT_LT(1u, pieces.size());
  EXPECT_EQ(2u, pieces.size());
  EXPECT_EQ("", pieces[0]);
  EXPECT_EQ("Hello World!", pieces[1]);
}

/////////////////////////////////////////////////
TEST(HelpersTest, SplitOneDelimiterAtEnd)
{
  char delim = '!';
  std::string orig = "Hello World!";
  std::vector<std::string> pieces = transport::split(orig, delim);
  ASSERT_LT(1u, pieces.size());
  EXPECT_EQ(2u, pieces.size());
  EXPECT_EQ("Hello World", pieces[0]);
  EXPECT_EQ("", pieces[1]);
}

//////////////////////////////////////////////////
/// \brief Check the getTransportImplementation() function.
TEST(HelpersTest, TransportImplementation)
{
  std::string impl = transport::getTransportImplementation();
  EXPECT_FALSE(impl.empty());

  ASSERT_TRUE(gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", "abc"));

  impl = transport::getTransportImplementation();
  EXPECT_EQ("abc", impl);

  // This call unsets the environment variable on Windows.
  ASSERT_TRUE(gz::utils::setenv("GZ_TRANSPORT_IMPLEMENTATION", ""));

  impl = transport::getTransportImplementation();
  std::string value;
  if (gz::utils::env("GZ_TRANSPORT_IMPLEMENTATION", value, true))
    EXPECT_TRUE(impl.empty());
  else
    EXPECT_EQ("zeromq", impl);
}

//////////////////////////////////////////////////
/// \brief waitUntil: predicate immediately true returns true near-instantly.
TEST(HelpersTest, WaitUntilImmediatelyTrue)
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
TEST(HelpersTest, WaitUntilNeverTrue)
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
TEST(HelpersTest, WaitUntilBecomesTrueMidWait)
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
TEST(HelpersTest, WaitUntilZeroTimeout)
{
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(0)));
  EXPECT_TRUE(transport::waitUntil([]{ return true; },
      std::chrono::milliseconds(0)));
}

//////////////////////////////////////////////////
/// \brief waitUntil: elapsed time is close to timeout on failure.
TEST(HelpersTest, WaitUntilElapsedCloseToTimeout)
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
TEST(HelpersTest, WaitForServiceNotAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;
  EXPECT_FALSE(transport::waitForService(node, "/nonexistent",
      std::chrono::milliseconds(200)));
}

//////////////////////////////////////////////////
/// \brief waitForService: service already advertised returns true.
TEST(HelpersTest, WaitForServiceAlreadyAdvertised)
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
TEST(HelpersTest, WaitForTopicAlreadyAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/test_topic");
  EXPECT_TRUE(pub);

  EXPECT_TRUE(transport::waitForTopic(node, "/test_topic",
      std::chrono::milliseconds(2000)));
}
