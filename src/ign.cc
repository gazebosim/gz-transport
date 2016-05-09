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
#include <ignition/msgs.hh>

#include "ignition/transport/config.hh"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Node.hh"

#ifdef _MSC_VER
# pragma warning(disable: 4503)
#endif

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicList()
{
  Node node;

  std::vector<std::string> topics;
  node.TopicList(topics);

  for (auto const &topic : topics)
    std::cout << topic << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicInfo(const char *_topic)
{
  if (!_topic || std::string(_topic).empty())
  {
    std::cerr << "Invalid topic. Topic must not be empty.\n";
    return;
  }

  Node node;

  // Get the publishers on the requested topic
  std::vector<MessagePublisher> publishers;
  node.TopicInfo(_topic, publishers);

  if (!publishers.empty())
  {
    std::cout << "Publishers [Address, Message Type]:\n";

    /// List the publishers
    for (std::vector<MessagePublisher>::iterator iter = publishers.begin();
        iter != publishers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
        << (*iter).MsgTypeName() << std::endl;
    }
  }
  else
  {
    std::cout << "No publishers on topic [" << _topic << "]\n";
  }

  // TODO: Add subscribers lists
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdServiceList()
{
  Node node;

  std::vector<std::string> services;
  node.ServiceList(services);

  for (auto const &service : services)
    std::cout << service << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdServiceInfo(const char *_service)
{
  if (!_service || std::string(_service).empty())
  {
    std::cerr << "Invalid service. Service must not be empty.\n";
    return;
  }

  Node node;

  // Get the publishers on the requested topic
  std::vector<ServicePublisher> publishers;
  node.ServiceInfo(_service, publishers);

  if (!publishers.empty())
  {
    std::cout << "Service providers [Address, Request Message Type, "
              << "Response Message Type]:\n";

    /// List the publishers
    for (std::vector<ServicePublisher>::iterator iter = publishers.begin();
        iter != publishers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
        << (*iter).ReqTypeName() << ", " << (*iter).RepTypeName()
        << std::endl;
    }
  }
  else
  {
    std::cout << "No service providers on service [" << _service << "]\n";
  }
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicPub(const char *_topic,
                                           const char *_msgType,
                                           const char *_msgData)
{
  if (!_topic)
  {
    std::cerr << "Topic name is null\n";
    return;
  }

  if (!_msgType)
  {
    std::cerr << "Message type is null\n";
    return;
  }

  if (!_msgData)
  {
    std::cerr << "Message data is null\n";
    return;
  }

  // Create the message, and populate the field with _msgData
  auto msg = ignition::msgs::Factory::New(_msgType, _msgData);
  if (msg)
  {
    // Create the node and advertise the topic
    ignition::transport::Node node;
    auto pubId = node.Advertise(_topic, msg->GetTypeName());

    // Publish the message
    if (pubId)
    {
      node.Publish(pubId, *msg);
    }
    else
    {
      std::cerr << "Unable to advertise on topic[" << _topic << "] "
        << "with message type[" << _msgType << "].\n";
    }
  }
  else
  {
    std::cerr << "Unable to create message of type[" << _msgType << "] "
      << "with data[" << _msgData << "].\n";
  }
}


//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE char *ignitionVersion()
{
  int majorVersion = IGNITION_TRANSPORT_MAJOR_VERSION;
  int minorVersion = IGNITION_TRANSPORT_MINOR_VERSION;
  int patchVersion = IGNITION_TRANSPORT_PATCH_VERSION;

  return ign_strdup((std::to_string(majorVersion) + "." +
                     std::to_string(minorVersion) + "." +
                     std::to_string(patchVersion)).c_str());
}
