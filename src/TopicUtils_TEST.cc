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

#include <limits.h>
#include <uuid/uuid.h>
#include <string>
#include "ignition/transport/TopicUtils.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the topic names.
TEST(TopicUtilsTest, testIsValidtopic)
{
  std::string invalid1 = "";
  std::string invalid2 = " ";
  std::string invalid3 = "topic1 ";
  std::string invalid4 = "abc//def";
  std::string invalid5 = "ab~cd";

  std::string valid1 = "~/abcde";
  std::string valid2 = "~abcde";

  EXPECT_TRUE(transport::TopicUtils::IsValidTopic(valid1));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic(valid2));

  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(invalid1));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(invalid2));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(invalid3));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(invalid4));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(invalid5));
}

//////////////////////////////////////////////////
/// \brief Check GetScopeName.
TEST(TopicUtilsTest, testGetScopeName)
{
  std::string ns1 = "";
  std::string ns2 = "abc";

  std::string t1 = "~/def";
  std::string t2 = "~def";
  std::string t3 = "/def";
  std::string t4 = "def";

  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns1, t1), "/abcdef");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns1, t2), "/abcdef");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns1, t3), "/abcdef");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns1, t4), "/abcdef");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns2, t1), "/abcdef");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns2, t2), "/abcdef");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns2, t3), "/def");
  EXPECT_EQ(transport::TopicUtils::GetScopedName(ns2, t4), "/def");
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
