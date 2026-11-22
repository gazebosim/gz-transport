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
#include <cstddef>
#include <string>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_config.hh"

using namespace gz;

static std::string g_topic = "/foo"; // NOLINT(*)
static std::string g_largeTopic = "/large_msg"; // NOLINT(*)
static std::string g_inPlaceTopic = "/in_place"; // NOLINT(*)
static std::string g_largeInPlaceTopic = "/large_msg_in_place"; // NOLINT(*)

//////////////////////////////////////////////////
/// \brief Publish _msg with PublishInPlace().
void publishInPlace(transport::Node::Publisher &_pub,
                    const google::protobuf::Message &_msg)
{
  const std::size_t size = _msg.ByteSizeLong();
  _pub.PublishInPlace(size, [&](char *_buffer)
  {
    return _msg.SerializeToArray(_buffer, static_cast<int>(size));
  });
}

//////////////////////////////////////////////////
/// \brief A publisher node.
void advertiseAndPublish()
{
  msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  // 256 KB — above the default 128 KB SHM threshold so it exercises
  // the SHM publish path when running with Zenoh.
  const std::size_t largePayloadSize = 256 * 1024;
  msgs::Bytes largeMsg;
  largeMsg.set_data(std::string(largePayloadSize, 'S'));

  transport::Node node;

  auto pub = node.Advertise<msgs::Vector3d>(g_topic);
  auto largePub = node.Advertise<msgs::Bytes>(g_largeTopic);
  // The same messages, serialized in place every 100 ms meanwhile: a heap
  // buffer handed over to the transport, or for the large one a Zenoh SHM
  // buffer once Zenoh has set up its pool (lazily, after the first ones).
  auto inPlacePub = node.Advertise<msgs::Vector3d>(g_inPlaceTopic);
  auto largeInPlacePub = node.Advertise<msgs::Bytes>(g_largeInPlaceTopic);
  auto waitPublishingInPlace = [&](int _ms)
  {
    for (int t = 0; t < _ms; t += 100)
    {
      publishInPlace(inPlacePub, msg);
      publishInPlace(largeInPlacePub, largeMsg);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  };

  waitPublishingInPlace(500);
  pub.Publish(msg);
  largePub.Publish(largeMsg);
  waitPublishingInPlace(1500);
  pub.Publish(msg);
  largePub.Publish(largeMsg);
  waitPublishingInPlace(1000);
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
