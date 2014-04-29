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

#include <google/protobuf/message.h>
#include <iostream>
#include <mutex>
#include <string>
#include "ignition/transport/Node.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

//////////////////////////////////////////////////
transport::Node::Node(bool _verbose)
  : dataPtr(transport::NodePrivate::GetInstance(_verbose))
{
}

//////////////////////////////////////////////////
transport::Node::~Node()
{
}

//////////////////////////////////////////////////
int transport::Node::Advertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  this->dataPtr.topics.SetAdvertisedByMe(_topic, true);

  for (auto addr : this->dataPtr.myAddresses)
    this->dataPtr.SendAdvertiseMsg(transport::AdvType, _topic, addr);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::UnAdvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  this->dataPtr.topics.SetAdvertisedByMe(_topic, false);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::Publish(const std::string &_topic,
                             const ProtoMsgPtr &_msgPtr)
{
  assert(_topic != "");

  if (this->dataPtr.topics.HasSubscribers(_topic))
  {
    std::string data;
    _msgPtr->SerializeToString(&data);
    if (this->dataPtr.Publish(_topic, data) != 0)
      return -1;
  }

  // Execute local callbacks
  return this->dataPtr.topics.RunLocalCallbacks(_topic, _msgPtr);
}

//////////////////////////////////////////////////
int transport::Node::SubscribeLocal(const std::string &_topic,
                                    const transport::CallbackLocal &_cb)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  if (this->dataPtr.verbose)
    std::cout << "\nSubscribe local(" << _topic << ")\n";

  // Register the local callback
  this->dataPtr.topics.AddLocalCallback(_topic, _cb);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::UnSubscribe(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr.mutex);

  if (this->dataPtr.verbose)
    std::cout << "\nUnubscribe (" << _topic << ")\n";

  this->dataPtr.topics.SetSubscribed(_topic, false);
  this->dataPtr.topics.SetCallback(_topic, nullptr);

  // Remove the filter for this topic
  this->dataPtr.subscriber->setsockopt(ZMQ_UNSUBSCRIBE, _topic.data(),
                                       _topic.size());
  return 0;
}
