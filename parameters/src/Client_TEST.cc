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

#include "ignition/transport/parameters/Client.hh"
#include "ignition/transport/parameters/Registry.hh"

#include <ignition/msgs/boolean.pb.h>
#include <ignition/msgs/stringmsg.pb.h>

#include "gtest/gtest.h"

using namespace ignition;
using namespace ignition::transport;
using namespace ignition::transport::parameters;

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

TEST_F(ParametersClientTest, Parameter)
{
  {
    ParametersClient client;
    {
      auto msg = client.Parameter("parameter1");
      EXPECT_NE(msg, nullptr);
      auto msgDownCasted = dynamic_cast<msgs::Boolean *>(msg.get());
      ASSERT_NE(msgDownCasted, nullptr);
      EXPECT_EQ(msgDownCasted->data(), false);
    }
    {
      msgs::StringMsg msg;
      client.Parameter("parameter2", msg);
      EXPECT_EQ(msg.data(), "");
    }
    {
      auto msg = client.Parameter<msgs::StringMsg>("parameter3");
      EXPECT_EQ(msg.data(), "asd");
    }
  }
  {
    ParametersClient client{"/ns"};
    {
      auto msg = client.Parameter("another_param1");
      EXPECT_NE(msg, nullptr);
      auto msgDownCasted = dynamic_cast<msgs::Boolean *>(msg.get());
      ASSERT_NE(msgDownCasted, nullptr);
      EXPECT_EQ(msgDownCasted->data(), false);
    }
    {
      auto msg = client.Parameter("another_param2");
      EXPECT_NE(msg, nullptr);
      auto msgDownCasted = dynamic_cast<msgs::StringMsg *>(msg.get());
      ASSERT_NE(msgDownCasted, nullptr);
      EXPECT_EQ(msgDownCasted->data(), "bsd");
    }
  }
}

TEST_F(ParametersClientTest, SetParameter)
{
  {
    ParametersClient client;
    msgs::StringMsg msg;
    msg.set_data("testing");
    client.SetParameter("parameter2", msg);
    auto param = registry_.Parameter<msgs::StringMsg>("parameter2");
    EXPECT_EQ(param.data(), "testing");
  }
  {
    ParametersClient client;
    msgs::Boolean msg;
    EXPECT_THROW(
      client.SetParameter("parameter2", msg),
      ParameterInvalidTypeException);
  }
  {
    ParametersClient client;
    msgs::Boolean msg;
    EXPECT_THROW(
      client.SetParameter("parameter_doesnt_exist", msg),
      ParameterNotDeclaredException);
  }
  {
    ParametersClient client{"/ns"};
    msgs::Boolean msg;
    msg.set_data(true);
    client.SetParameter("another_param1", msg);
    auto param = registry_other_ns_.Parameter<msgs::Boolean>("another_param1");
    EXPECT_EQ(param.data(), true);
  }
}

//////////////////////////////////////////////////
TEST_F(ParametersClientTest, DeclareParameter)
{
  {
    ParametersClient client;
    msgs::StringMsg msg;
    msg.set_data("declaring");
    client.DeclareParameter("new_parameter", msg);
    auto param = registry_.Parameter<msgs::StringMsg>("new_parameter");
    EXPECT_EQ(param.data(), "declaring");
  }
  {
    ParametersClient client;
    msgs::Boolean msg;
    EXPECT_THROW(
      client.DeclareParameter("parameter1", msg),
      ParameterAlreadyDeclaredException);
  }
  {
    ParametersClient client{"/ns"};
    msgs::Boolean msg;
    msg.set_data(true);
    client.DeclareParameter("new_parameter", msg);
    auto param = registry_other_ns_.Parameter<msgs::Boolean>("new_parameter");
    EXPECT_EQ(param.data(), true);
  }
}

TEST_F(ParametersClientTest, ListParameters)
{
  {
    ParametersClient client;
    auto declarations = client.ListParameters();
    bool foundParam1{false};
    bool foundParam2{false};
    bool foundParam3{false};
    for (auto decl : declarations.parameter_declarations()) {
      if (decl.name() == "parameter1" && decl.type() == "ign_msgs.Boolean") {
        foundParam1 = true;
      }
      if (decl.name() == "parameter2" && decl.type() == "ign_msgs.StringMsg") {
        foundParam2 = true;
      }
      if (decl.name() == "parameter3" && decl.type() == "ign_msgs.StringMsg") {
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
        decl.name() == "another_param1" && decl.type() == "ign_msgs.Boolean")
      {
        foundParam1 = true;
      }
      if (
        decl.name() == "another_param2" && decl.type() == "ign_msgs.StringMsg")
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
