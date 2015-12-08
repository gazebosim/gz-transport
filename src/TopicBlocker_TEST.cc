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

#include <chrono>
#include <string>
#include <thread>
#include "ignition/transport/TopicBlocker.hh"
#include "ignition/transport/test_config.h"
#include "gtest/gtest.h"

using namespace ignition;

transport::TopicBlocker blocker;
unsigned int clientTimeout = 2000;

//////////////////////////////////////////////////
void releaseTask()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(clientTimeout));

  blocker.Release();
}

//////////////////////////////////////////////////
/// \brief Check the accessors.
TEST(TopicBlockerTest, accessors)
{
  std::thread clientThread;
  unsigned int timeout = 5000;

  auto start = std::chrono::system_clock::now();

  // Start the thread that releases the topic blocker after some time.
  clientThread = std::thread(&releaseTask);
  EXPECT_TRUE(blocker.Wait(timeout));

  clientThread.join();

  auto end = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
    (end - start).count();

  // Check if the elapsed time was close to the timeout.
  EXPECT_NEAR(elapsed, clientTimeout, 10.0);
}

//////////////////////////////////////////////////
/// \brief .
TEST(TopicBlockerTest, timeout)
{
  unsigned int timeout = 5000;

  auto start = std::chrono::system_clock::now();

  // Start the thread that releases the topic blocker after some time.
  EXPECT_FALSE(blocker.Wait(timeout));

  auto end = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
    (end - start).count();

  // Check if the elapsed time was close to the timeout.
  EXPECT_NEAR(elapsed, timeout, 10.0);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
