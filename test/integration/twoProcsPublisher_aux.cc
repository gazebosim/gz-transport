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
#include <cstdlib>
#include <string>
#include <ignition/msgs.hh>

#include "gz/transport/Node.hh"
#include "gz/transport/test_config.h"

using namespace gz;

static std::string g_topic = "/foo"; // NOLINT(*)

//////////////////////////////////////////////////
/// \brief A publisher node.
/// \param[in] _keepAliveSec Number of additional seconds to keep the node
/// alive (still advertising and sending discovery heartbeats) after the two
/// messages have been published. This lets slow discovery consumers, e.g.
/// `ign topic -i/-l` on a heavily loaded machine where process startup plus the
/// ~2s discovery initialization can otherwise outlast a short-lived publisher,
/// reliably discover this publisher (see issue #887). When 0 (the default) the
/// node exits promptly, which the waitAndCleanupFork-based tests rely on.
void advertiseAndPublish(int _keepAliveSec)
{
  msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  transport::Node node;

  auto pub = node.Advertise<msgs::Vector3d>(g_topic);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  pub.Publish(msg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  pub.Publish(msg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Optionally remain alive so slow consumers can still discover us. The
  // parent process terminates us once it is done, so this is an upper bound.
  if (_keepAliveSec > 0)
    std::this_thread::sleep_for(std::chrono::seconds(_keepAliveSec));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Partition name has not be passed as argument" << std::endl;
    return -1;
  }

  // Set the partition name for this test.
  setenv("IGN_PARTITION", argv[1], 1);

  // Optionally stay alive after publishing. The fork-based test helpers cannot
  // pass extra arguments, so this is controlled via an environment variable
  // that the parent sets only when it needs a long-lived publisher.
  int keepAliveSec = 0;
  if (const char *keepAlive = std::getenv("IGN_TRANSPORT_TEST_KEEP_ALIVE_SEC"))
    keepAliveSec = std::atoi(keepAlive);

  advertiseAndPublish(keepAliveSec);
}
