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

#include <cstdio>
#include <string>

#include <ignition/msgs.hh>
#include <ignition/transport.hh>

//////////////////////////////////////////////////
/// \brief Provide an "echo" service.
void srvEcho(const std::string &_topic, const ignition::msgs::StringMsg &_req,
  ignition::msgs::StringMsg &_rep, bool &_result)
{
  // Set the response's content.
  _rep.set_data(_req.data());

  // The response succeed.
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Provide an "echo" service.
void srvEcho2(const std::string &_topic, const ignition::msgs::CmdVel2D &_req,
  ignition::msgs::CmdVel2D &_rep, bool &_result)
{
  std::cout << "Input parameters:" << std::endl;
  std::cout << _req.DebugString() << std::endl;

  // Set the response's content.
  _rep.set_velocity(_req.velocity());
  _rep.set_theta(_req.theta());

  // The response succeed.
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Provide an "echo" service.
void srvEcho3(const std::string &_topic, const ignition::msgs::Int &_req,
  ignition::msgs::Int &_rep, bool &_result)
{
  std::cout << "Input parameters:" << std::endl;
  std::cout << _req.DebugString() << std::endl;

  // Set the response's content.
  _rep.set_data(_req.data());

  // The response succeed.
  _result = true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Create a transport node.
  ignition::transport::Node node;

  // Advertise a service call.
  // node.Advertise("/echo", srvEcho);
  node.Advertise("/echo", srvEcho2);

  // Wait for requests.
  getchar();
}
