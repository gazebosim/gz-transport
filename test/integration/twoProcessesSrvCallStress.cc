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
#include <cstdlib>
#include <string>
#include "ignition/transport/Node.hh"
#include "gtest/gtest.h"
#include "msg/int.pb.h"

using namespace ignition;

std::string topic = "/foo";


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
TEST(twoProcSrvCall, ThousandCalls)
{
  pid_t pid = fork();

  if (pid == 0)
  {
    transport::Node node;
    EXPECT_TRUE(node.Advertise(topic, srvEcho));
    while (1) {};
  }
  else
  {
    transport::msgs::Int req;
    transport::msgs::Int response;
    bool result;
    unsigned int timeout = 1000;
    transport::Node node;

    for (int i = 0; i < 15000; i++)
    {
      req.set_data(i);
      EXPECT_TRUE(node.Request(topic, req, timeout, response, result));

      // Check the service response.
      EXPECT_TRUE(result);
      EXPECT_EQ(i, response.data());
    }
  }
  // kill the child
  kill(pid, SIGTERM);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
