/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <zmq.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <vector>

#include "ignition/transport/Discovery.hh"

/////////////////////////////////////////////////
bool ignition::transport::pollSockets(const std::vector<int> &_sockets,
                                      const int _timeout)
{
  zmq::pollitem_t items[] =
  {
    {0, _sockets.at(0), ZMQ_POLLIN, 0},
  };

  try
  {
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), _timeout);
  }
  catch(...)
  {
    return false;
  }

  // Return if we got a reply.
  return items[0].revents & ZMQ_POLLIN;
}
