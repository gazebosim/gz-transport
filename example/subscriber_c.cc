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
#include <ignition/msgs/stringmsg.pb.h>
#include <ignition/transport/CIface.h>

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const char *_data, const size_t _size, const char *_msgType)
{
  ignition::msgs::StringMsg msg;
  msg.ParseFromArray(_data, _size);

  printf("Msg length: %ld bytes\n", _size);
  printf("Msg type: %s\n", _msgType);
  printf("Msg contents: %s\n", msg.data().c_str());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Create a transport node.
  IgnTransportNode node = ignTransportInit();
  const char *topic = "/foo";

  // Subscribe to a topic by registering a callback.
  if (!ignTransportSubscribe(node, topic, cb))
  {
    printf("Error subscribing to topic %s.\n", topic);
    return -1;
  }

  // Zzzzzz.
  ignTransportWaitForShutdown();

  return 0;
}
