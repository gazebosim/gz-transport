/*
 * Copyright (C) 2019 Open Source Robotics Foundation
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

#include <memory>
#include <vector>

#include "ignition/transport/Node.hh"
#include "ignition/transport/CIface.h"

// All of the nodes.
static std::vector<std::unique_ptr<ignition::transport::Node>> nodes;

// Map of the publishers.
// Key: pair of node Id and topic name. A node Id is generated from the
// ignTransportInit function.
// Value: A node publisher.
static std::map<std::pair<IgnTransportNode, const char *>,
  ignition::transport::Node::Publisher> publishers;

/////////////////////////////////////////////////
IgnTransportNode ignTransportInit(void)
{
  nodes.push_back(std::make_unique<ignition::transport::Node>());
  return nodes.size() - 1;
}

/////////////////////////////////////////////////
int ignTransportPublish(IgnTransportNode _node, const char *_topic,
                         const void *_data, const char *_msgType)
{
  if (_node >= 0 && _node < static_cast<int>(nodes.size()))
  {
    std::pair<IgnTransportNode, const char *> key(_node, _topic);

    // Create a publisher if one does not exist.
    if (publishers.find(key) == publishers.end())
      publishers[key] = nodes[_node]->Advertise(_topic, _msgType);

    // Publishe the message.
    return publishers[key].PublishRaw(
        reinterpret_cast<const char*>(_data), _msgType) ? 0 : 1;
  }

  return 1;
}

/////////////////////////////////////////////////
int ignTransportSubscribe(IgnTransportNode _node, const char *_topic,
    void (*_callback)(const char *, const size_t, const char *))
{
  if (_node >= 0 && _node < static_cast<int>(nodes.size()))
  {
    return nodes[_node]->SubscribeRaw(_topic,
        [_callback](const char *_msg,
          const size_t _size,
          const ignition::transport::MessageInfo &_info) -> void {
        _callback(_msg, _size, _info.Type().c_str());
        }) ? 0 : 1;
  }

  return 1;
}

/////////////////////////////////////////////////
void ignTransportWaitForShutdown()
{
  ignition::transport::waitForShutdown();
}
