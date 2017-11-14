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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <ignition/msgs.hh>
#include <ignition/transport.hh>

using namespace std::chrono_literals;

/// \brief Forward declaration
void onDestroy(ignition::msgs::StringMsg *_msg);

/////////////////////
/// Global variables.
/////////////////////

/// \brief Mutex used with the condition variable.
static std::mutex g_mutex;

/// \brief Condition variable for waiting until the message is recycled.
static std::condition_variable g_cv;

/// \brief Flag used to break the publisher loop and terminate the program.
static std::atomic<bool> g_terminatePub(false);

/// \brief Declare a global message.
static std::unique_ptr<
  ignition::msgs::StringMsg, void(*)(ignition::msgs::StringMsg*)> g_msg(
    new ignition::msgs::StringMsg, onDestroy);

///////////////////
/// Free functions.
///////////////////

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
/// \brief Custom deallocation function.
void onDestroy(ignition::msgs::StringMsg *_msg)
{
  // Recycle the message.
  if (!g_terminatePub)
  {
    std::unique_lock<std::mutex> lk(g_mutex);
    g_msg.reset(_msg);
    g_cv.notify_all();
    return;
  }

  // It's time to leave, deallocate the message.
  if (_msg)
    delete _msg;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const ignition::msgs::StringMsg &_msg)
{
  std::cout << "Msg: " << _msg.data() << std::endl << std::endl;
}

//////////////////////////////////////////////////
/// \brief Main program.
int main(int argc, char **argv)
{
  // Install a signal handler for SIGINT and SIGTERM.
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  // Create a transport node and advertise a topic.
  ignition::transport::Node node;
  std::string topic = "/foo";

  auto pub = node.Advertise<ignition::msgs::StringMsg>(topic);
  if (!pub)
  {
    std::cerr << "Error advertising topic [" << topic << "]" << std::endl;
    return -1;
  }

  if (!node.Subscribe(topic, cb))
  {
    std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
    return -1;
  }

  // Option 1: Publish messages at 1Hz copying the message each time.
  ignition::msgs::StringMsg msg;
  msg.set_data("HELLO");

  while (!g_terminatePub)
  {
    if (!pub.Publish((msg)))
      break;

    std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  // Option 2: Publish messages at 1Hz allocating a message each time.
  // while (!g_terminatePub)
  // {
  //   auto msg = std::make_unique<ignition::msgs::StringMsg>();
  //   msg->set_data("HELLO");
  //   if (!pub.Publish(std::move(msg)))
  //     break;
  //
  //   std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
  //   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  // }

  // Option 3: Publish messages at 1Hz recycling the message.
  // g_msg->set_data("HELLO");
  //
  // while (!g_terminatePub)
  // {
  //   if (!pub.Publish(std::move(g_msg)))
  //     break;
  //
  //   // Wait until we get the ownership of the message back.
  //   {
  //      std::unique_lock<std::mutex> lk(g_mutex);
  //      if (!g_cv.wait_until(lk, std::chrono::system_clock::now() + 100ms,
  //        []{return g_msg != nullptr;}))
  //        {
  //           std::cerr << "I didn't get the message back." << std::endl;
  //           g_terminatePub = true;
  //        }
  //   }
  //
  //   std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
  //   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  // }

  // Option 4: Publish messages at 1Hz synchronously.
  // ignition::msgs::StringMsg msg;
  // msg.set_data("HELLO");
  //
  // while (!g_terminatePub)
  // {
  //   if (!pub.PublishSync((msg)))
  //     break;
  //
  //   std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
  //   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  // }

  return 0;
}
