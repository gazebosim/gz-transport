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
#include <csignal>
#include <ignition/transport.hh>
#include "msgs/stringmsg.pb.h"
#include "msgs/stringmsg2.pb.h"

bool terminatePub = false;

//////////////////////////////////////////////////
/// \brief Function callback executed when a SIGINT or SIGTERM signals are
/// captured. This is used to break the infinite loop that publishes messages
/// and exit the program smoothly.
void signal_handler(int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
    terminatePub = true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Install a signal handler for SIGINT.
  std::signal(SIGINT, signal_handler);

  // Create a transport node and advertise a topic.
  ignition::transport::Node node;
  std::string topic = "/foo";

  // if (!node.Advertise<example::msgs::StringMsg>(topic))
  // {
  //   std::cerr << "Error advertising topic [" << topic << "]" << std::endl;
  //   return -1;
  // }

  if (!node.Advertise<example::msgs::StringMsg2>(topic))
  {
    std::cerr << "Error advertising topic [" << topic << "]" << std::endl;
    return -1;
  }

  // Prepare the message.
  // example::msgs::StringMsg msg;
  // msg.set_data("HELLO");

  example::msgs::StringMsg2 msg2;
  msg2.set_data("HELLO2");

  // Publish messages at 1Hz.
  while (!terminatePub)
  {
    // auto res1 = node.Publish(topic, msg);
    auto res2 = node.Publish(topic, msg2);
    // if ((!res1) && (!res2))
    if (!res2)
      break;

    std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  return 0;
}
