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

#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <ignition/transport.hh>
#include "msgs/stringmsg.pb.h"

std::mutex mutex;

//////////////////////////////////////////////////
void request()
{
  std::lock_guard<std::mutex> lock(mutex);

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  // Create a transport node.
  ignition::transport::Node node;

  // Prepare the input parameters.
  example::msgs::StringMsg req;
  req.set_data("HELLO");

  example::msgs::StringMsg rep;
  bool result;
  unsigned int timeout = 5000;

  // Request the "/echo" service.
  bool executed = node.Request("/echo", req, timeout, rep, result);

  if (executed)
  {
    if (result)
      std::cout << "Response: [" << rep.data() << "]" << std::endl;
    else
      std::cout << "Service call failed" << std::endl;
  }
  else
    std::cerr << "Service call timed out" << std::endl;
}


//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const example::msgs::StringMsg &_msg)
{
  std::lock_guard<std::mutex> lock(mutex);
  std::cout << "Msg: " << _msg.data() << std::endl << std::endl;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ignition::transport::Node node;
  std::string topic = "/foo";

  // Subscribe to a topic by registering a callback.
  if (!node.Subscribe(topic, cb))
  {
    std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
    return -1;
  }

  request();

  // Zzzzzz.
  std::cout << "Press <ENTER> to exit" << std::endl;
  getchar();

  return 0;
}
