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

#include "ignition/transport/parameters/Registry.hh"

#include <ignition/msgs/boolean.pb.h>
#include <ignition/msgs/stringmsg.pb.h>

#include "gtest/gtest.h"

using namespace ignition;
using namespace ignition::transport;
using namespace ignition::transport::parameters;

//////////////////////////////////////////////////
TEST(ParametersRegistry, ConstructDestruct)
{
  ParametersRegistry registry{""};
}

//////////////////////////////////////////////////
TEST(ParametersRegistry, DeclareParameter)
{
  ParametersRegistry registry{""};
  EXPECT_THROW(
    registry.DeclareParameter("will_fail", nullptr),
    std::invalid_argument);
  registry.DeclareParameter("parameter1", std::make_unique<ignition::msgs::Boolean>());
  EXPECT_THROW(
    registry.DeclareParameter("parameter1", std::make_unique<ignition::msgs::Boolean>()),
    ParameterAlreadyDeclaredException);
}

//////////////////////////////////////////////////
TEST(ParametersRegistry, Parameter)
{
  ParametersRegistry registry{""};
  EXPECT_THROW(
    registry.Parameter("will_fail"),
    ParameterNotDeclaredException);
  registry.DeclareParameter(
    "parameter1", std::make_unique<ignition::msgs::Boolean>());
  auto param = registry.Parameter("parameter1");
  EXPECT_NE(param, nullptr);
  auto booleanParam = registry.Parameter<ignition::msgs::Boolean>("parameter1");
  auto upcasted = dynamic_cast<ignition::msgs::Boolean *>(param.get());
  EXPECT_EQ(upcasted->data(), booleanParam.data());
  ASSERT_NE(upcasted, nullptr);
  EXPECT_THROW(
    registry.Parameter<ignition::msgs::StringMsg>("parameter1"),
    ParameterInvalidTypeException);
}

//////////////////////////////////////////////////
TEST(ParametersRegistry, SetParameter)
{
  ParametersRegistry registry{""};
  EXPECT_THROW(
    registry.SetParameter("will_fail", std::make_unique<ignition::msgs::Boolean>()),
    ParameterNotDeclaredException);
  registry.DeclareParameter(
    "parameter1", std::make_unique<ignition::msgs::Boolean>());
  auto unique = std::make_unique<ignition::msgs::Boolean>();
  unique->set_data(true);
  registry.SetParameter("parameter1", std::move(unique));
  auto readValue = registry.Parameter<ignition::msgs::Boolean>("parameter1");
  EXPECT_EQ(readValue.data(), true);
  ignition::msgs::Boolean msg;
  msg.set_data(false);
  registry.SetParameter("parameter1", msg);
  readValue = registry.Parameter<ignition::msgs::Boolean>("parameter1");
  EXPECT_EQ(readValue.data(), false);
  EXPECT_THROW(
    registry.SetParameter("parameter1", std::make_unique<ignition::msgs::StringMsg>()),
    ParameterInvalidTypeException);
}

//////////////////////////////////////////////////
TEST(ParametersRegistry, ListParameters)
{
  ParametersRegistry registry{""};
  auto declarations = registry.ListParameters();
  EXPECT_EQ(declarations.parameter_declarations_size(), 0);
  registry.DeclareParameter(
    "parameter1", std::make_unique<ignition::msgs::Boolean>());
  registry.DeclareParameter(
    "parameter2", std::make_unique<ignition::msgs::StringMsg>());
  declarations = registry.ListParameters();
  EXPECT_EQ(declarations.parameter_declarations_size(), 2);
  bool foundParam1 = false;
  bool foundParam2 = false;
  for (auto decl : declarations.parameter_declarations()) {
    if (decl.name() == "parameter1" && decl.type() == "ign_msgs.Boolean") {
      foundParam1 = true;
    }
    if (decl.name() == "parameter2" && decl.type() == "ign_msgs.StringMsg") {
      foundParam2 = true;
    }
  }
  EXPECT_TRUE(foundParam1) << "expected to find declaration for parameter1";
  EXPECT_TRUE(foundParam2) << "expected to find declaration for parameter2";
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
