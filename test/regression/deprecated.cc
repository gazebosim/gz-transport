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

#include "gtest/gtest.h"

#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

static std::string partition;
static std::string g_FQNPartition;
static std::string g_topic = "/foo";

using Request = ignition::msgs::UInt64;
using Reply = ignition::msgs::UInt64;

//////////////////////////////////////////////////
void addOne(const Request &_request, Reply &_reply, bool &_result)
{
  _reply = _request;
  _reply.set_data(_request.data()+1);
  _result = true;
}

void returnTwo(Reply &_reply, bool &_result)
{
  _reply.set_data(2);
  _result = true;
}

//////////////////////////////////////////////////
TEST(deprecated, NodeAdvertise)
{
  ignition::transport::Node requester;
  Request request;
  Reply reply;
  bool result = false;

  auto reset = [&]()
  {
    request.set_data(0);
    reply.set_data(0);
    result = false;
  };

  request.set_data(1);

  // Call Node::Advertise on each of the deprecated overloads to make sure that
  // they still work.
  {
    ignition::transport::Node service;
    service.Advertise(g_topic, &addOne);

    EXPECT_TRUE(requester.Request(g_topic, request, 100, reply, result));
    EXPECT_EQ(request.data() + 1, reply.data());
    EXPECT_TRUE(result);
  }

  reset();

  {
    ignition::transport::Node service;
    service.Advertise(g_topic, &returnTwo);

    EXPECT_TRUE(requester.Request(g_topic, 100, reply, result));
    EXPECT_EQ(2u, reply.data());
    EXPECT_TRUE(result);
  }

  reset();

  {
    std::function<void(const Request &, Reply &, bool&)> addTwo =
        [](const Request &_request, Reply &_reply, bool &_result)
    {
      _reply.set_data(_request.data() + 2);
      _result = true;
    };

    ignition::transport::Node service;
    service.Advertise(g_topic, addTwo);

    EXPECT_TRUE(requester.Request(g_topic, request, 100, reply, result));
    EXPECT_EQ(request.data() + 2, reply.data());
    EXPECT_TRUE(result);
  }

  reset();

  {
    std::function<void(Reply &, bool &)> returnThree =
        [](Reply &_reply, bool &_result)
    {
      _reply.set_data(3);
      _result = true;
    };

    ignition::transport::Node service;
    service.Advertise(g_topic, returnThree);

    EXPECT_TRUE(requester.Request(g_topic, 100, reply, result));
    EXPECT_EQ(3u, reply.data());
    EXPECT_TRUE(result);
  }

  reset();

  {
    /// \brief Dummy class for testing the signature of Node::Advertise that
    /// takes a class member function with Request and Reply arguments.
    class Adder
    {
      /// \brief Reply by adding num to the request message.
      /// \param[in] _request The number that num will be added to
      /// \param[out] _reply The result of the adding
      /// \param[out] _result Always true
      public: void addNum(const Request &_request, Reply &_reply,
                          bool &_result)
      {
        _reply.set_data(_request.data() + num);
        _result = true;
      }

      /// \brief The number to add to the request message.
      public: unsigned int num;
    };

    Adder adder;
    adder.num = 3;

    ignition::transport::Node service;
    service.Advertise(g_topic, &Adder::addNum, &adder);

    EXPECT_TRUE(requester.Request(g_topic, request, 100, reply, result));
    EXPECT_EQ(request.data() + adder.num, reply.data());
    EXPECT_TRUE(result);
  }

  reset();

  {
    /// \brief Dummy class for testing the signature of Node::Advertise that
    /// takes a class member function with just a Reply argument.
    class Getter
    {
      /// \brief Reply by adding num to the request message.
      /// \param[out] _reply The result of the adding
      /// \param[out] _result Always true
      public: void getNum(Reply &_reply, bool &_result)
      {
        _reply.set_data(num);
        _result = true;
      }

      /// \brief The number to return
      public: unsigned int num;
    };

    Getter getter;
    getter.num = 4;

    ignition::transport::Node service;
    service.Advertise(g_topic, &Getter::getNum, &getter);

    EXPECT_TRUE(requester.Request(g_topic, 100, reply, result));
    EXPECT_EQ(getter.num, reply.data());
    EXPECT_TRUE(result);
  }
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();
  g_FQNPartition = std::string("/") + partition;

  // Set the partition name for this process.
  setenv("IGN_PARTITION", partition.c_str(), 1);

  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
