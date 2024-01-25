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
#include <cstdlib>
#include <string>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gtest/gtest.h"

#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

using twoProcSrvCallWithoutOuput = testing::PartitionedTransportTest;

static std::string g_topic = "/foo"; // NOLINT(*)

//////////////////////////////////////////////////
TEST_F(twoProcSrvCallWithoutOuput, ThousandCalls)
{
  this->SpawnSubprocess({
    test_executables::kTwoProcsSrvCallWithoutOutputReplierInc});

  msgs::Int32 req;
  transport::Node node;

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  for (int i = 0; i < 15000; i++)
  {
    req.set_data(i);
    ASSERT_TRUE(node.Request(g_topic, req));
  }
}
