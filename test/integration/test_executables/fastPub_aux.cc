/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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
#include <string>
#include <gz/msgs/vector3d.pb.h>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

using namespace gz;

static std::string g_topic = "/foo"; // NOLINT(*)

//////////////////////////////////////////////////
/// \brief A publisher node.
void advertiseAndPublish()
{
  msgs::Vector3d msg;
  msg.set_x(1.0);
  msg.set_y(2.0);
  msg.set_z(3.0);

  transport::Node node;

  auto pub = node.Advertise<msgs::Vector3d>(g_topic);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  pub.Publish(msg);
}

//////////////////////////////////////////////////
int main(int, char **)
{
  advertiseAndPublish();
}
