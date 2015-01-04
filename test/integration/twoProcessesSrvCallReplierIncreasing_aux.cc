/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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
#include <climits>
#include <string>
#include "ignition/transport/Node.hh"
#include "msg/int.pb.h"
#include "gtest/gtest.h"
#include "ignition/transport/test_config.h"

using namespace ignition;

std::string partition = "testPartition";
std::string ns = "";
std::string topic = "/foo";
int Forever = INT_MAX;

//////////////////////////////////////////////////
/// \brief Provide a service.
void srvEcho(const std::string &_topic, const transport::msgs::Int &_req,
  transport::msgs::Int &_rep, bool &_result)
{
  EXPECT_EQ(_topic, topic);
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
void runReplier()
{
  transport::Node node(partition, ns);
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Run the node forever. Should be killed by the test that uses this.
  std::this_thread::sleep_for(std::chrono::milliseconds(Forever));
}

//////////////////////////////////////////////////
TEST(twoProcSrvCallReplierAux, SrvProcReplier)
{
  runReplier();
}
