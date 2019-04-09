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

#include <map>
#include <memory>

#include "ignition/transport/Node.hh"
#include "ignition/transport/CIface.h"

/// \brief A wrapper to store an Ignition Transport node and its publishers.
struct IgnTransportNode
{
  /// \brief Pointer to the node.
  std::unique_ptr<ignition::transport::Node> nodePtr;

  /// \brief All publishers of this node.
  std::map<std::string, ignition::transport::Node::Publisher> publishers;
};

/////////////////////////////////////////////////
IgnTransportNode *ignTransportNodeCreate()
{
  IgnTransportNode *ignTransportNode = new IgnTransportNode();
  ignTransportNode->nodePtr = std::make_unique<ignition::transport::Node>();
  return ignTransportNode;
}

/////////////////////////////////////////////////
void ignTransportNodeDestroy(IgnTransportNode *_node)
{
  if (_node)
  {
    delete _node;
    _node = nullptr;
  }
}

/////////////////////////////////////////////////
int ignTransportPublish(IgnTransportNode *_node, const char *_topic,
    const void *_data, const char *_msgType)
{
  if (!_node)
    return 1;

  // Create a publisher if one does not exist.
  if (_node->publishers.find(_topic) == _node->publishers.end())
    _node->publishers[_topic] = _node->nodePtr->Advertise(_topic, _msgType);

  // Publish the message.
  return _node->publishers[_topic].PublishRaw(
    reinterpret_cast<const char*>(_data), _msgType) ? 0 : 1;
}

/////////////////////////////////////////////////
int ignTransportSubscribe(IgnTransportNode *_node, const char *_topic,
    void (*_callback)(const char *, const size_t, const char *))
{
  if (!_node)
    return 1;

  return _node->nodePtr->SubscribeRaw(_topic,
      [_callback](const char *_msg,
                  const size_t _size,
                  const ignition::transport::MessageInfo &_info) -> void
                  {
                    _callback(_msg, _size, _info.Type().c_str());
                  }) ? 0 : 1;
}

/////////////////////////////////////////////////
void ignTransportWaitForShutdown()
{
  ignition::transport::waitForShutdown();
}
