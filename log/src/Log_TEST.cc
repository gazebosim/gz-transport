/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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
#include <unordered_set>

#include "ignition/transport/log/Log.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace ignition::transport;
using namespace std::chrono_literals;

//////////////////////////////////////////////////
TEST(Log, OpenMemoryDatabase)
{
  log::Log logFile;
  EXPECT_TRUE(logFile.Open(":memory:", std::ios_base::out));
}


//////////////////////////////////////////////////
TEST(Log, OpenReadOnlyMemoryDatabase)
{
  log::Log logFile;
  EXPECT_FALSE(logFile.Open(":memory:", std::ios_base::in));
  EXPECT_FALSE(logFile.Valid());
}

//////////////////////////////////////////////////
TEST(Log, MoveConstructor)
{
  log::Log logFile;
  EXPECT_TRUE(logFile.Open(":memory:", std::ios_base::out));
  log::Log logDest(std::move(logFile));
  EXPECT_TRUE(logDest.Valid());
  EXPECT_FALSE(logFile.Valid());
}

//////////////////////////////////////////////////
TEST(Log, UnopenedLog)
{
  log::Log logFile;
  EXPECT_FALSE(logFile.Valid());
  EXPECT_EQ(std::string(""), logFile.Version());
  EXPECT_EQ(nullptr, logFile.Descriptor());
  char data[] = {1, 2, 3, 4};
  EXPECT_FALSE(logFile.InsertMessage(common::Time::Zero, "/foo/bar", ".fiz.buz",
    reinterpret_cast<const void *>(data), 4));

  auto batch = logFile.QueryMessages();
  EXPECT_EQ(batch.end(), batch.begin());
}

//////////////////////////////////////////////////
TEST(Log, OpenMemoryDatabaseTwice)
{
  log::Log logFile;
  EXPECT_TRUE(logFile.Open(":memory:", std::ios_base::out));
  EXPECT_FALSE(logFile.Open(":memory:", std::ios_base::out));
}

//////////////////////////////////////////////////
TEST(Log, OpenImpossibleFileName)
{
  log::Log logFile;
  EXPECT_FALSE(logFile.Open("///////////", std::ios_base::out));
}

//////////////////////////////////////////////////
TEST(Log, InsertMessage)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  std::string data("Hello World");

  EXPECT_TRUE(logFile.InsertMessage(
      common::Time(),
      "/some/topic/name",
      "some.message.type",
      reinterpret_cast<const void *>(data.c_str()),
      data.size()));
}

//////////////////////////////////////////////////
TEST(Log, AllMessagesNone)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto batch = logFile.QueryMessages();
  EXPECT_EQ(batch.end(), batch.begin());
}

//////////////////////////////////////////////////
TEST(Log, InsertMessageGetMessages)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  std::string data1("first_data");
  std::string data2("second_data");

  EXPECT_TRUE(logFile.InsertMessage(
      common::Time(1, 0),
      "/some/topic/name",
      "some.message.type",
      reinterpret_cast<const void *>(data1.c_str()),
      data1.size()));

  EXPECT_TRUE(logFile.InsertMessage(
      common::Time(2, 0),
      "/some/topic/name",
      "some.message.type",
      reinterpret_cast<const void *>(data2.c_str()),
      data2.size()));

  auto batch = logFile.QueryMessages();
  auto iter = batch.begin();
  ASSERT_NE(batch.end(), iter);
  EXPECT_EQ(data1, iter->Data());
  ++iter;
  ASSERT_NE(batch.end(), iter);
  EXPECT_EQ(data2, iter->Data());
  ++iter;
  EXPECT_EQ(log::MsgIter(), iter);
}

//////////////////////////////////////////////////
TEST(Log, QueryMessagesByTopicNone)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  std::unordered_set<std::string> noTopics;
  auto batch = logFile.QueryMessages(
        log::TopicList::Create(noTopics));

  EXPECT_EQ(batch.end(), batch.begin());
}

//////////////////////////////////////////////////
TEST(Log, Insert2Get1MessageByTopic)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  std::string data1("first_data");
  std::string data2("second_data");

  EXPECT_TRUE(logFile.InsertMessage(
      common::Time(1, 0),
      "/some/topic/name",
      "some.message.type",
      reinterpret_cast<const void *>(data1.c_str()),
      data1.size()));

  EXPECT_TRUE(logFile.InsertMessage(
      common::Time(2, 0),
      "/second/topic/name",
      "some.message.type",
      reinterpret_cast<const void *>(data2.c_str()),
      data2.size()));

  auto batch = logFile.QueryMessages(
        log::TopicList("/some/topic/name"));

  auto iter = batch.begin();
  ASSERT_NE(batch.end(), iter);
  EXPECT_EQ(data1, iter->Data());
  ++iter;
  EXPECT_EQ(log::MsgIter(), iter);
}

//////////////////////////////////////////////////
TEST(Log, CheckVersion)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));
  EXPECT_EQ("0.1.0", logFile.Version());
}

//////////////////////////////////////////////////
TEST(Log, NullDescriptorUnopenedLog)
{
  log::Log logFile;
  EXPECT_EQ(nullptr, logFile.Descriptor());
}


//////////////////////////////////////////////////
/// \brief container for test message data
typedef struct
{
  common::Time time;
  std::string topic;
  std::string type;
  std::string data;
} TestMessage;

//////////////////////////////////////////////////
std::vector<TestMessage> StandardTestMessages()
{
  std::vector<TestMessage> messages;

  messages.push_back({
      common::Time(1, 0),
      "/topic/one",
      "msg.type.1",
      "topic1_type1_num1",
      });
  messages.push_back({
      common::Time(1, 2.5e8),
      "/topic/one",
      "msg.type.2",
      "topic1_type2_num1",
      });
  messages.push_back({
      common::Time(1, 7.5e8),
      "/topic/two",
      "msg.type.1",
      "topic2_type1_num1",
      });
  messages.push_back({
      common::Time(2, 0),
      "/topic/one",
      "msg.type.1",
      "topic1_type1_num2",
      });
  messages.push_back({
      common::Time(2, 2.5e8),
      "/topic/one",
      "msg.type.2",
      "topic1_type2_num2",
      });
  messages.push_back({
      common::Time(2, 7.5e8),
      "/topic/two",
      "msg.type.2",
      "topic2_type1_num2",
      });
  messages.push_back({
      common::Time(3, 0),
      "/topic/one",
      "msg.type.1",
      "topic1_type1_num3",
      });
  messages.push_back({
      common::Time(3, 2.5e8),
      "/topic/one",
      "msg.type.2",
      "topic1_type2_num3",
      });
  messages.push_back({
      common::Time(3, 7.5e8),
      "/topic/two",
      "msg.type.1",
      "topic2_type1_num3",
      });

  return messages;
}


void CheckEquality(const TestMessage &_golden, const log::Message &_uut)
{
  EXPECT_EQ(_golden.time, _uut.TimeReceived());
  EXPECT_EQ(_golden.topic, _uut.Topic());
  EXPECT_EQ(_golden.type, _uut.Type());
  EXPECT_EQ(_golden.data, _uut.Data());
}

//////////////////////////////////////////////////
void InsertMessages(log::Log &_log,
    const std::vector<TestMessage> &_messages)
{
  for (const TestMessage &msg : _messages)
  {
    ASSERT_TRUE(_log.InsertMessage(
        msg.time,
        msg.topic,
        msg.type,
        reinterpret_cast<const void *>(msg.data.c_str()),
        msg.data.size()));
  }
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsAfterInclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Inclusive);
  auto batch = logFile.QueryMessages(
      log::AllTopics(log::QualifiedTimeRange::From(beginTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 3;
  auto uutIter = batch.begin();
  for (; goldenIter != testMessages.end() && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(6u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsAfterInclusiveCopy)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Inclusive);
  log::AllTopics options_orig(log::QualifiedTimeRange::From(beginTime));
  log::AllTopics options(options_orig);
  auto batch = logFile.QueryMessages(options);

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 3;
  auto uutIter = batch.begin();
  for (; goldenIter != testMessages.end() && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(6u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsAfterInclusiveMove)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Inclusive);
  log::AllTopics options_orig(log::QualifiedTimeRange::From(beginTime));
  log::AllTopics options(std::move(options_orig));
  auto batch = logFile.QueryMessages(options);

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 3;
  auto uutIter = batch.begin();
  for (; goldenIter != testMessages.end() && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(6u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsAfterExclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Exclusive);
  auto batch = logFile.QueryMessages(
      log::AllTopics(log::QualifiedTimeRange::From(beginTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 4;
  auto uutIter = batch.begin();
  for (; goldenIter != testMessages.end() && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(5u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsBeforeInclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime endTime(3s, log::QualifiedTime::Qualifier::Inclusive);
  auto batch = logFile.QueryMessages(
      log::AllTopics(log::QualifiedTimeRange::Until(endTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin();
  auto uutIter = batch.begin();
  for (; goldenIter != (testMessages.end() - 2) && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(7u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsBeforeExclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime endTime(3s, log::QualifiedTime::Qualifier::Exclusive);
  auto batch = logFile.QueryMessages(
      log::AllTopics(log::QualifiedTimeRange::Until(endTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin();
  auto uutIter = batch.begin();
  for (; goldenIter != (testMessages.end() - 3) && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(6u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsBetweenInclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Inclusive);
  log::QualifiedTime endTime(3s, log::QualifiedTime::Qualifier::Inclusive);
  auto batch = logFile.QueryMessages(
      log::AllTopics(log::QualifiedTimeRange(beginTime, endTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 3;
  auto uutIter = batch.begin();
  for (; goldenIter != (testMessages.end() - 2) && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(4u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryAllTopicsBetweenExclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Exclusive);
  log::QualifiedTime endTime(3s, log::QualifiedTime::Qualifier::Exclusive);
  auto batch = logFile.QueryMessages(
      log::AllTopics(log::QualifiedTimeRange(beginTime, endTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 4;
  auto uutIter = batch.begin();
  for (; goldenIter != (testMessages.end() - 3) && uutIter != batch.end();
       ++goldenIter, ++uutIter, ++num_msgs)
  {
    CheckEquality(*goldenIter, *uutIter);
  }
  EXPECT_EQ(2u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryTopicPatternAllTime)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  auto batch = logFile.QueryMessages(log::TopicPattern(std::regex(".*/two")));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin();
  auto uutIter = batch.begin();
  for (; goldenIter != testMessages.end() && uutIter != batch.end();
       ++goldenIter)
  {
    if (goldenIter->topic == "/topic/two")
    {
      CheckEquality(*goldenIter, *uutIter);
      ++uutIter;
      ++num_msgs;
    }
  }
  EXPECT_EQ(3u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryTopicPatternAfterInclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime beginTime(2s, log::QualifiedTime::Qualifier::Inclusive);
  auto batch = logFile.QueryMessages(log::TopicPattern(
        std::regex(".*/one"), log::QualifiedTimeRange::From(beginTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin() + 3;
  auto uutIter = batch.begin();
  for (; goldenIter != testMessages.end() && uutIter != batch.end();
       ++goldenIter)
  {
    if (goldenIter->topic == "/topic/one")
    {
      CheckEquality(*goldenIter, *uutIter);
      ++uutIter;
      ++num_msgs;
    }
  }
  EXPECT_EQ(4u, num_msgs);
}

//////////////////////////////////////////////////
TEST(Log, QueryTopicPatternBeforeInclusive)
{
  log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", std::ios_base::out));

  auto testMessages = StandardTestMessages();
  InsertMessages(logFile, testMessages);

  log::QualifiedTime endTime(3s, log::QualifiedTime::Qualifier::Inclusive);
  auto batch = logFile.QueryMessages(log::TopicPattern(
        std::regex(".*/one"), log::QualifiedTimeRange::Until(endTime)));

  std::size_t num_msgs = 0;
  auto goldenIter = testMessages.begin();
  auto uutIter = batch.begin();
  for (; goldenIter != (testMessages.end() - 2) && uutIter != batch.end();
       ++goldenIter)
  {
    if (goldenIter->topic == "/topic/one")
    {
      CheckEquality(*goldenIter, *uutIter);
      ++uutIter;
      ++num_msgs;
    }
  }
  EXPECT_EQ(5u, num_msgs);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
