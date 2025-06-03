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

#include "gz/transport/parameters/Client.hh"
#include "gz/transport/parameters/Registry.hh"

#include <gz/msgs/boolean.pb.h>
#include <gz/msgs/stringmsg.pb.h>

#include "gtest/gtest.h"

using namespace gz;
using namespace transport;
using namespace transport::parameters;

class ParametersClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_.DeclareParameter("parameter1", std::make_unique<msgs::Boolean>());
    registry_.DeclareParameter(
      "parameter2", std::make_unique<msgs::StringMsg>());
    auto anotherStrMsg = std::make_unique<msgs::StringMsg>();
    anotherStrMsg->set_data("asd");
    registry_.DeclareParameter("parameter3", std::move(anotherStrMsg));
    registry_other_ns_.DeclareParameter(
      "another_param1", std::make_unique<msgs::Boolean>());
    anotherStrMsg = std::make_unique<msgs::StringMsg>();
    anotherStrMsg->set_data("bsd");
    registry_other_ns_.DeclareParameter(
      "another_param2", std::move(anotherStrMsg));
  }

  void TearDown() override {}

  ParametersRegistry registry_{""};
  ParametersRegistry registry_other_ns_{"/ns"};
};

//////////////////////////////////////////////////
TEST(ParametersClient, ConstructDestruct)
{
  ParametersClient client;
}

//////////////////////////////////////////////////
TEST_F(ParametersClientTest, Parameter)
{
  {
    ParametersClient client;
    {
      msgs::Boolean msg;
      ParameterResult result = client.Parameter("parameter1", msg);
      EXPECT_TRUE(result) << result;
      EXPECT_EQ(msg.data(), false);
    }
    {
      std::unique_ptr<google::protobuf::Message> msg;
      ParameterResult result = client.Parameter("parameter2", msg);
      EXPECT_TRUE(result) << result;
      ASSERT_NE(nullptr, msg);
      auto downcastedMsg = dynamic_cast<msgs::StringMsg *>(msg.get());
      EXPECT_EQ(downcastedMsg->data(), "");
    }
    {
      msgs::Boolean msg;
      auto result = client.Parameter("parameter3", msg);
      EXPECT_FALSE(result) << result;
      EXPECT_EQ(result.ResultType(), ParameterResultType::InvalidType)
          << result;
      EXPECT_EQ(result.ParamName(), "parameter3") << result;
    }
  }
  {
    ParametersClient client{"/ns"};
    {
      msgs::Boolean msg;
      auto result = client.Parameter("another_param1", msg);
      EXPECT_TRUE(result) << result;
      EXPECT_EQ(msg.data(), false);
    }
    {
      msgs::StringMsg msg;
      auto result = client.Parameter("another_param2", msg);
      EXPECT_TRUE(result) << result;
      EXPECT_EQ(msg.data(), "bsd");
    }
  }
}

//////////////////////////////////////////////////
TEST_F(ParametersClientTest, SetParameter)
{
  {
    ParametersClient client;
    msgs::StringMsg msg;
    msg.set_data("testing");
    {
      auto result = client.SetParameter("parameter2", msg);
      EXPECT_TRUE(result) << result;
    }
    msgs::StringMsg msg_got;
    {
      auto result = registry_.Parameter("parameter2", msg_got);
      EXPECT_TRUE(result) << result;
    }
    EXPECT_EQ(msg_got.data(), "testing");
  }
  {
    ParametersClient client;
    msgs::Boolean msg;
    auto result = client.SetParameter("parameter2", msg);
    EXPECT_FALSE(result) << result;
    EXPECT_EQ(result.ResultType(), ParameterResultType::InvalidType) << result;
    EXPECT_EQ(result.ParamName(), "parameter2") << result;
  }
  {
    ParametersClient client;
    msgs::Boolean msg;
    auto result = client.SetParameter("parameter_doesnt_exist", msg);
    EXPECT_FALSE(result) << result;
    EXPECT_EQ(result.ResultType(), ParameterResultType::NotDeclared) << result;
    EXPECT_EQ(result.ParamName(), "parameter_doesnt_exist") << result;
  }
  {
    ParametersClient client{"/ns"};
    msgs::Boolean msg;
    msg.set_data(true);
    {
      auto result = client.SetParameter("another_param1", msg);
      EXPECT_TRUE(result) << result;
    }
    msgs::Boolean msg_got;
    {
      auto result = registry_other_ns_.Parameter("another_param1", msg_got);
      EXPECT_TRUE(result) << result;
    }
    EXPECT_EQ(msg_got.data(), true);
  }
}

//////////////////////////////////////////////////
TEST_F(ParametersClientTest, DeclareParameter)
{
  {
    ParametersClient client;
    msgs::StringMsg msg;
    msg.set_data("declaring");
    {
      auto result = client.DeclareParameter("new_parameter", msg);
      EXPECT_TRUE(result) << result;
    }
    msgs::StringMsg msg_got;
    {
      auto result = registry_.Parameter("new_parameter", msg_got);
      EXPECT_TRUE(result) << result;
    }
    EXPECT_EQ(msg_got.data(), "declaring");
  }
  {
    ParametersClient client;
    msgs::Boolean msg;
    auto result = client.DeclareParameter("parameter1", msg);
    EXPECT_FALSE(result);
    EXPECT_EQ(result.ResultType(), ParameterResultType::AlreadyDeclared);
  }
  {
    ParametersClient client{"/ns"};
    msgs::Boolean msg;
    msg.set_data(true);
    msgs::Boolean msg_got;
    {
      auto result = client.DeclareParameter("new_parameter", msg);
      EXPECT_TRUE(result) << result;
    }
    {
      auto result = registry_other_ns_.Parameter("new_parameter", msg_got);
      EXPECT_TRUE(result) << result;
    }
    EXPECT_EQ(msg_got.data(), true);
  }
}

//////////////////////////////////////////////////
TEST_F(ParametersClientTest, ListParameters)
{
  {
    ParametersClient client;
    auto declarations = client.ListParameters();
    bool foundParam1{false};
    bool foundParam2{false};
    bool foundParam3{false};
    for (auto decl : declarations.parameter_declarations()) {
      if (decl.name() == "parameter1" && decl.type() == "gz.msgs.Boolean") {
        foundParam1 = true;
      }
      if (decl.name() == "parameter2" && decl.type() == "gz.msgs.StringMsg") {
        foundParam2 = true;
      }
      if (decl.name() == "parameter3" && decl.type() == "gz.msgs.StringMsg") {
        foundParam3 = true;
      }
    }
    EXPECT_TRUE(foundParam1) << "expected to find declaration for parameter1";
    EXPECT_TRUE(foundParam2) << "expected to find declaration for parameter2";
    EXPECT_TRUE(foundParam3) << "expected to find declaration for parameter3";
  }
  {
    ParametersClient client{"/ns"};
    auto declarations = client.ListParameters();
    bool foundParam1{false};
    bool foundParam2{false};
    for (auto decl : declarations.parameter_declarations()) {
      if (
        decl.name() == "another_param1" && decl.type() == "gz.msgs.Boolean")
      {
        foundParam1 = true;
      }
      if (
        decl.name() == "another_param2" && decl.type() == "gz.msgs.StringMsg")
      {
        foundParam2 = true;
      }
    }
    EXPECT_TRUE(foundParam1)
      << "expected to find declaration for another_param1";
    EXPECT_TRUE(foundParam2)
      << "expected to find declaration for another_param2";
  }
}
