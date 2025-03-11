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

//! [complete]
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <gz/msgs.hh>
#include <gz/transport.hh>

/// \brief Flag used to break the publisher loop and terminate the program.
static std::atomic<bool> g_terminatePub(false);

//////////////////////////////////////////////////
/// \brief Function callback executed when a SIGINT or SIGTERM signals are
/// captured. This is used to break the infinite loop that publishes messages
/// and exit the program smoothly.
void signal_handler(int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
    g_terminatePub = true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Install a signal handler for SIGINT and SIGTERM.
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  // Create a transport node and advertise a topic.
  gz::transport::Node node;
  std::string topic = "/foo";


  while (!g_terminatePub)
  {
    std::cout << "Enter any key to publish message " << std::endl;
    int character;
    character = getchar();

    auto pub = node.Advertise<gz::msgs::StringMsg>(topic);
    if (!pub)
    {
      std::cerr << "Error advertising topic [" << topic << "]" << std::endl;
      return -1;
    }

    // Uncomment the sleep below and the message will successfully
    // reach the subscriber
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!pub.HasConnections())
    {
      std::cout << "No connections found. " << std::endl;
      continue;
    }

    // Prepare the message.
    gz::msgs::StringMsg msg;
    msg.set_data("HELLO");

    if (!pub.Publish(msg))
    {
      std::cout << "Failed to publish message.. " << std::endl;
    }
    else
    {
      std::cout << "Successfully published message." << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
//! [complete]
