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
#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <climits>
#include <string>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "gtest/gtest.h"

using namespace gz;

static std::string g_topic = "/foo"; // NOLINT(*)
static int g_data = 5;
static int kForever = INT_MAX;

//////////////////////////////////////////////////
/// \brief Provide a service without input.
bool srvWithoutInput(msgs::Int32 &_rep)
{
  _rep.set_data(g_data);
  return true;
}

//////////////////////////////////////////////////
void runReplier()
{
  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_topic, srvWithoutInput));

  // Run the node forever. Should be killed by the test that uses this.
  std::this_thread::sleep_for(std::chrono::milliseconds(kForever));
}

//////////////////////////////////////////////////
TEST(twoProcSrvCallWithoutInputReplierAux, SrvProcReplier)
{
  runReplier();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
