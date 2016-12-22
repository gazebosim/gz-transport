/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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
#include <ignition/msgs.hh>
#include "gtest/gtest.h"
#include "ignition/transport/Node.hh"
#include "ignition/transport/test_config.h"

using namespace ignition;

static int data = 1;
static std::string g_topic = "/foo";

/// \brief Function is called everytime a topic update is received.
void cb(const ignition::msgs::Int32 &_msg)
{
  EXPECT_EQ(_msg.data(), data);
}

//////////////////////////////////////////////////
//// \brief A Subscriber Node
int main(int argc, char **argv)
{	
  ignition::transport::Node node;
  
  EXPECT_TRUE(node.Subscribe(g_topic, cb));
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  
  return 0;
}
