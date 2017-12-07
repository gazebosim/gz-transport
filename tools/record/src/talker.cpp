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

// Copy/pasted from transport tutorials

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <ignition/msgs.hh>
#include <ignition/transport.hh>

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
  ignition::transport::Node node;

  std::string topicString = "/some/string";
  std::string topicFloat = "/some/float";
  auto pubString = node.Advertise<ignition::msgs::StringMsg>(topicString);
  auto pubFloat = node.Advertise<ignition::msgs::Float>(topicFloat);

  if (!pubString || !pubFloat)
  {
    std::cerr << "Error advertising topic\n";
    return -1;
  }


  // Publish messages at 1Hz.
  float i = 3.14;
  while (!g_terminatePub)
  {
    // Prepare the messages 
    ignition::msgs::StringMsg msgString;
    msgString.set_data("HELLO");
    ignition::msgs::Float msgFloat;
    i += 7.004;
    msgFloat.set_data(i);


    if (!pubString.Publish(msgString))
      break;
    if (!pubFloat.Publish(msgFloat))
      break;

    std::cout << "Publishing\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
