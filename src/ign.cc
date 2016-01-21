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
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>
#include "ignition/transport/config.hh"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"
#include "msgs/ign_string.pb.h"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const msgs::IgnString &_msg)
{
  std::cout << _msg.data() << std::endl << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdTopicList()
{
  Node node;

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> topics;
  node.TopicList(topics);

  for (auto const &topic : topics)
    std::cout << topic << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdTopicEcho(const char *_topic,
  const double _duration)
{
  Node node;

  std::string topic(_topic);
  if (!node.Subscribe(topic + "_TEXT", cb))
  {
    std::cerr << "CmdTopicEcho(): Error in Subscribe() to topic ["
              << topic + "_TEXT]" << std::endl;
    return;
  }

  if (_duration < 0)
    getchar();
  else
    std::this_thread::sleep_for(std::chrono::milliseconds(
      static_cast<int64_t>(_duration * 1000)));
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdServiceList()
{
  Node node;

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> services;
  node.ServiceList(services);

  for (auto const &service : services)
    std::cout << service << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE char *ignitionVersion()
{
  int majorVersion = IGNITION_TRANSPORT_MAJOR_VERSION;
  int minorVersion = IGNITION_TRANSPORT_MINOR_VERSION;
  int patchVersion = IGNITION_TRANSPORT_PATCH_VERSION;

  return strdup((std::to_string(majorVersion) + "." +
                 std::to_string(minorVersion) + "." +
                 std::to_string(patchVersion)).c_str());
}
