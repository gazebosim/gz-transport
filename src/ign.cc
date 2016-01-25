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
#include "ignition/transport/config.hh"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

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
extern "C" IGNITION_VISIBLE void cmdTopicInfo(const char *_topic)
{
  Node node;
  NodeOptions opts;
  NodeShared *shared = NodeShared::GetInstance();

  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(opts.Partition(),
    "", std::string(_topic), fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return;
  }

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  MsgAddresses_M pubs;
  shared->discovery->MsgPublishers(fullyQualifiedTopic, pubs);

  msgs::IgnString req;
  msgs::IgnString rep;
  bool result;
  unsigned int timeout = 5000;

  req.set_data(_topic);

  for (auto const &proc : pubs)
  {
    std::cout << "Process UUID: [" << proc.first << "]" << std::endl;
    for (auto const &pub : proc.second)
    {
      std::string service = "_INTERNAL_" + pub.NUuid();

      if (!node.Request(service, req, timeout, rep, result))
        continue;

      std::cout << "  End point: [" << pub.Addr() << "]" << std::endl;
      std::cout << "  Node UUID: [" << pub.NUuid() << "]" << std::endl;
      std::cout << "  Type: [" << pub.MsgTypeName() << "]" << std::endl;
      std::cout << "  Subscribers:" << std::endl;
      std::cout << rep.data() << std::endl;
    }
  }
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
