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

#include <string>
#include "ignition/transport/TopicUtils.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the topic names.
TEST(TopicUtilsTest, testTopics)
{
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("abc"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("abc/de"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("a"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("abc/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc/d"));
  EXPECT_TRUE(transport::TopicUtils::IsValidTopic("/abc/d/e"));

  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(""));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic(" "));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("~a"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("topic1 "));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("abc//def"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("ab~cd"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("/"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("~/"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("~"));
  EXPECT_FALSE(transport::TopicUtils::IsValidTopic("@partition"));
}

//////////////////////////////////////////////////
/// \brief Check the topic namespace.
TEST(TopicUtilsTest, testNamespaces)
{
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("abcde"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("abcde/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde/fg"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace("/abcde/fg/"));
  EXPECT_TRUE(transport::TopicUtils::IsValidNamespace(""));

  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace(" "));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("ns "));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("abc//def"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("ab~cd"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("~/abcde"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("~abcde"));
  EXPECT_FALSE(transport::TopicUtils::IsValidNamespace("@namespace"));
}

//////////////////////////////////////////////////
/// \brief Check GetScopeName.
TEST(TopicUtilsTest, testGetFullyQualifiedName)
{
  std::string p0 = "@partition";
  std::string p1 = "@partition/@";
  std::string p2 = "@@";
  std::string p3 = "partition";
  std::string p4 = "";

  std::string ns0 = "~ns";
  std::string ns1 = "";
  std::string ns2 = "abc";
  std::string t1 = "~/def";
  std::string t2 = "~def";
  std::string t3 = "/def";
  std::string t4 = "def/";
  std::string t5 = "def/ghi";
  std::string t6 = "def/ghi/";
  std::string t7 = "~/def/";
  std::string t8 = "~def/";
  std::string name;

  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns0, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns1, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p0, ns2, t8, name));

  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns0, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns1, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p1, ns2, t8, name));

  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns0, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns1, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p2, ns2, t8, name));

  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns0, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t2, name));
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t3, name));
  EXPECT_EQ(name, "@/partition@msg@/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t4, name));
  EXPECT_EQ(name, "@/partition@msg@/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t5, name));
  EXPECT_EQ(name, "@/partition@msg@/def/ghi");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t6, name));
  EXPECT_EQ(name, "@/partition@msg@/def/ghi");
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns1, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t2, name));
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t3, name));
  EXPECT_EQ(name, "@/partition@msg@/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t4, name));
  EXPECT_EQ(name, "@/partition@msg@/abc/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t5, name));
  EXPECT_EQ(name, "@/partition@msg@/abc/def/ghi");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t6, name));
  EXPECT_EQ(name, "@/partition@msg@/abc/def/ghi");
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p3, ns2, t8, name));

  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t2, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t3, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t4, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t5, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t6, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns0, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t2, name));
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t3, name));
  EXPECT_EQ(name, "@@msg@/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t4, name));
  EXPECT_EQ(name, "@@msg@/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t5, name));
  EXPECT_EQ(name, "@@msg@/def/ghi");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t6, name));
  EXPECT_EQ(name, "@@msg@/def/ghi");
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns1, t8, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t1, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t2, name));
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t3, name));
  EXPECT_EQ(name, "@@msg@/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t4, name));
  EXPECT_EQ(name, "@@msg@/abc/def");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t5, name));
  EXPECT_EQ(name, "@@msg@/abc/def/ghi");
  EXPECT_TRUE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t6, name));
  EXPECT_EQ(name, "@@msg@/abc/def/ghi");
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t7, name));
  EXPECT_FALSE(transport::TopicUtils::GetFullyQualifiedName(p4, ns2, t8, name));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
