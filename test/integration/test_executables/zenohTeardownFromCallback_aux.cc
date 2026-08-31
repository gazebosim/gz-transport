/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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

// Auxiliary binary for INTEGRATION_zenohTeardownFromCallback.
//
// Advertises a service whose callback unadvertises the service
// itself. With the weak_ptr capture used by the Zenoh handlers, the
// reply lambda then holds the last reference to the handler, so the
// handler (and its zenoh::Queryable) is destroyed on the Zenoh
// callback thread, from within the queryable's own callback. If
// that destruction undeclares inline, zenoh-c's wait_callbacks
// parks the thread on its own invocation and this process silently
// stops serving queries, even though it looks healthy and exits
// cleanly. The driving gtest detects this by requesting a second
// service afterwards.

#include <gz/msgs/int32.pb.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "gz/transport/Node.hh"
#include "gz/transport/WaitHelpers.hh"

#include <gz/utils/Environment.hh>

static std::atomic<bool> g_done{false};
static gz::transport::Node *g_node = nullptr;

//////////////////////////////////////////////////
/// \brief Service callback that tears down its own service.
bool SrvCb(const gz::msgs::Int32 &_req, gz::msgs::Int32 &_rep)
{
  _rep.set_data(_req.data());
  g_node->UnadvertiseSrv("/teardown_cb_srv");
  g_done = true;
  return true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Partition name has not been passed as argument"
              << std::endl;
    return -1;
  }
  gz::utils::setenv("GZ_PARTITION", argv[1]);

  gz::transport::Node node;
  g_node = &node;

  if (!node.Advertise("/teardown_cb_srv", SrvCb))
    return -1;

  // Wait for the remote request to arrive and the callback to
  // finish, bounded.
  if (!gz::transport::waitUntil([] { return g_done.load(); },
        std::chrono::seconds(20), std::chrono::milliseconds(50)))
  {
    std::cerr << "zenohTeardownFromCallback_aux: callback never ran"
              << std::endl;
    return 2;
  }

  // Liveness: the first queryable was just destroyed on a Zenoh
  // callback thread. Serve a second service from this same process;
  // if that thread parked in an inline undeclare, the second request
  // never reaches us and the driver times out.
  std::atomic<bool> second{false};
  std::function<bool(const gz::msgs::Int32 &, gz::msgs::Int32 &)> cb2 =
    [&second](const gz::msgs::Int32 &_req, gz::msgs::Int32 &_rep) -> bool
    {
      _rep.set_data(_req.data() + 1);
      second = true;
      return true;
    };
  if (!node.Advertise("/teardown_cb_srv2", cb2))
    return 3;

  if (!gz::transport::waitUntil([&second] { return second.load(); },
        std::chrono::seconds(15), std::chrono::milliseconds(50)))
  {
    std::cerr << "zenohTeardownFromCallback_aux: second service never "
              << "called (Zenoh callback thread parked?)" << std::endl;
    return 4;
  }
  std::cout << "zenohTeardownFromCallback_aux: OK" << std::endl;
  return 0;
}
