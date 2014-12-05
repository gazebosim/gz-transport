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
#include <iostream>
#include <vector>
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
/// \brief External hook to execute 'ign topic list' command from the command
/// line.
extern "C" IGNITION_VISIBLE void cmdTopicList()
{
  Node node;

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> topics;
  node.GetTopicList(topics);

  for (auto const &topic : topics)
    std::cout << topic << std::endl;
}

/// \brief External hook to execute 'ign service list' command from the command
/// line.
extern "C" IGNITION_VISIBLE void cmdServiceList()
{
  Node node;

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> services;
  node.GetServiceList(services);

  for (auto const &service : services)
    std::cout << service << std::endl;
}
