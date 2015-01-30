/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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
#include "ignition/transport/ServiceResult.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

static const std::string kExceptionMessage = "exception message";

//////////////////////////////////////////////////
/// \brief Check the getters and setters.
TEST(ServiceResultTest, BasicHeaderAPI)
{
  // Check default values.
  ServiceResult res;
  EXPECT_TRUE(res.ReturnCode() == Result_t::Pending);
  EXPECT_TRUE(res.ExceptionMsg().empty());
  EXPECT_FALSE(res.Succeed());
  EXPECT_FALSE(res.Failed());
  EXPECT_FALSE(res.Raised());

  // Setters and getters.
  res.ReturnCode(Result_t::Success);
  EXPECT_TRUE(res.ReturnCode() == Result_t::Success);
  EXPECT_TRUE(res.ExceptionMsg().empty());
  EXPECT_TRUE(res.Succeed());
  EXPECT_FALSE(res.Failed());
  EXPECT_FALSE(res.Raised());

  res.ReturnCode(Result_t::Fail);
  EXPECT_TRUE(res.ReturnCode() == Result_t::Fail);
  EXPECT_TRUE(res.ExceptionMsg().empty());
  EXPECT_FALSE(res.Succeed());
  EXPECT_TRUE(res.Failed());
  EXPECT_FALSE(res.Raised());

  res.ReturnCode(Result_t::Exception);
  res.ExceptionMsg(kExceptionMessage);
  EXPECT_TRUE(res.ReturnCode() == Result_t::Exception);
  EXPECT_EQ(res.ExceptionMsg(), kExceptionMessage);
  EXPECT_FALSE(res.Succeed());
  EXPECT_FALSE(res.Failed());
  EXPECT_TRUE(res.Raised());

  // Copy constructor.
  ServiceResult otherResult(res);
  EXPECT_TRUE(otherResult.ReturnCode() == Result_t::Exception);
  EXPECT_EQ(otherResult.ExceptionMsg(), kExceptionMessage);
  EXPECT_FALSE(otherResult.Succeed());
  EXPECT_FALSE(otherResult.Failed());
  EXPECT_TRUE(otherResult.Raised());

  // Copy assignment operator.
  ServiceResult anotherResult = res;
  EXPECT_TRUE(anotherResult.ReturnCode() == Result_t::Exception);
  EXPECT_EQ(anotherResult.ExceptionMsg(), kExceptionMessage);
  EXPECT_FALSE(anotherResult.Succeed());
  EXPECT_FALSE(anotherResult.Failed());
  EXPECT_TRUE(anotherResult.Raised());
}
