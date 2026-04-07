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
#include <gz/msgs/bytes.pb.h>
#include <gz/msgs/vector3d.pb.h>

#include <chrono>
#include <string>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_config.hh"
#include "ShmHelpers.hh"

using namespace gz;

static std::string g_topic = "/foo"; // NOLINT(*)
static std::string g_largeTopic = "/large_msg"; // NOLINT(*)

//////////////////////////////////////////////////
/// \brief A publisher node.
void advertiseAndPublish()
{
  msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  // Payload above the SHM threshold so it exercises the SHM publish
  // path when running with Zenoh.
  // Above the SHM threshold so it exercises the SHM path.
  const std::size_t largePayloadSize =
    transport::kDefaultShmThreshold * 2;
  msgs::Bytes largeMsg;
  largeMsg.set_data(std::string(largePayloadSize, 'S'));

  transport::Node node;

  auto pub = node.Advertise<msgs::Vector3d>(g_topic);
  auto largePub = node.Advertise<msgs::Bytes>(g_largeTopic);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  pub.Publish(msg);
  largePub.Publish(largeMsg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  pub.Publish(msg);
  largePub.Publish(largeMsg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
  gz::utils::setenv("GZ_PARTITION", argv[1]);

  advertiseAndPublish();
}
