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

#include <iostream>

#include "ignition/transport/DiscoveryOptions.hh"
#include "ignition/transport/Helpers.hh"
#include "gtest/gtest.h"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
/// \brief Check the default constructor.
TEST(DiscoveryOptionsTest, defConstructor)
{
  DiscoveryOptions opts;
  EXPECT_FALSE(opts.Verbose());
}

//////////////////////////////////////////////////
/// \brief Check the copy constructor.
TEST(DiscoveryOptionsTest, copyConstructor)
{
  DiscoveryOptions opts1;
  opts1.SetVerbose(true);
  DiscoveryOptions opts2(opts1);
  EXPECT_EQ(opts2.Verbose(), opts1.Verbose());
}

//////////////////////////////////////////////////
/// \brief Check the accessors.
TEST(DiscoveryOptionsTest, accessors)
{
  // Verbose.
  DiscoveryOptions opts;
  EXPECT_FALSE(opts.Verbose());
  opts.SetVerbose(true);
  EXPECT_TRUE(opts.Verbose());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
