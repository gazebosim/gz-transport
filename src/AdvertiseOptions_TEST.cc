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

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/test_config.h"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
/// \brief Check the copy constructor.
TEST(AdvertiseOptionsTest, copyConstructor)
{
  transport::AdvertiseOptions opts1;
  opts1.SetScope(transport::Scope_t::HOST);
  transport::AdvertiseOptions opts2(opts1);
  EXPECT_EQ(opts2.Scope(), opts1.Scope());
}

//////////////////////////////////////////////////
/// \brief Check the assignment operator.
TEST(AdvertiseOptionsTest, assignmentOp)
{
  transport::AdvertiseOptions opts1;
  transport::AdvertiseOptions opts2;
  opts1.SetScope(transport::Scope_t::PROCESS);
  opts2 = opts1;
  EXPECT_EQ(opts2.Scope(), opts1.Scope());
}

//////////////////////////////////////////////////
/// \brief Check the accessors.
TEST(AdvertiseOptionsTest, accessors)
{
  // Scope.
  transport::AdvertiseOptions opts;
  EXPECT_EQ(opts.Scope(), transport::Scope_t::ALL);
  opts.SetScope(transport::Scope_t::HOST);
  EXPECT_EQ(opts.Scope(), transport::Scope_t::HOST);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
