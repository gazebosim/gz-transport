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

#include "ignition/transport/log/Log.hh"
#include "gtest/gtest.h"

using namespace ignition;

//////////////////////////////////////////////////
TEST(Log, OpenMemoryDatabase)
{
  transport::log::Log logFile;
  EXPECT_TRUE(logFile.Open(":memory:", transport::log::READ_WRITE_CREATE));
}

//////////////////////////////////////////////////
TEST(Log, OpenImpossibleFileName)
{
  transport::log::Log logFile;
  EXPECT_FALSE(logFile.Open("///////////", transport::log::READ_WRITE_CREATE));
}

//////////////////////////////////////////////////
TEST(Log, InsertMessage)
{
  transport::log::Log logFile;
  ASSERT_TRUE(logFile.Open(":memory:", transport::log::READ_WRITE_CREATE));

  EXPECT_TRUE(logFile.InsertMessage(
      common::Time(),
      "/some/topic/name",
      "some.message.type",
      reinterpret_cast<const void *>("Hello World"),
      sizeof("Hello World")));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
