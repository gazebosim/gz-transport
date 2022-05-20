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
#include <stdio.h>
#include <gz/msgs/stringmsg.pb.h>
#include <gz/transport/CIface.h>

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const char *_data, const size_t _size, const char *_msgType,
        void *_userData)
{
  gz::msgs::StringMsg msg;
  msg.ParseFromArray(_data, _size);
  const char *partition;

  if (_userData)
    partition = static_cast<const char *>(_userData);
  else
    partition = "null";

  printf("Partition[%s] Msg length: %zu bytes\n", partition, _size);
  printf("Partition[%s] Msg type: %s\n", partition, _msgType);
  printf("Partition[%s] Msg contents: %s\n", partition, msg.data().c_str());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  const char *partName = "red";
  // Create a transport node.
  IgnTransportNode *node = ignTransportNodeCreate(nullptr);
  IgnTransportNode *nodeRed = ignTransportNodeCreate(partName);

  const char *topic = "/foo";

  // Subscribe to a topic by registering a callback.
  if (ignTransportSubscribe(node, topic, cb, nullptr) != 0)
  {
    printf("Error subscribing to topic %s.\n", topic);
    return -1;
  }

  // Subscribe to a topic by registering a callback.
  if (ignTransportSubscribe(nodeRed, topic, cb,
      const_cast<char*>(partName)) != 0)
  {
    printf("Error subscribing to topic %s.\n", topic);
    return -1;
  }

  // Zzzzzz.
  ignTransportWaitForShutdown();
  ignTransportNodeDestroy(&node);
  ignTransportNodeDestroy(&nodeRed);

  return 0;
}
