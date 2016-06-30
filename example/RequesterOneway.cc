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

#include <iostream>
#include <ignition/transport.hh>
#include <ignition/msgs.hh>

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Create a transport node.
  ignition::transport::Node node;

  // Prepare the input parameters.
  example::msgs::StringMsg req;
  req.set_data("HELLO");

  // Request the "/echo" service.
  bool executed = node.Request("/echo", req);

  if (executed)
  {
    std::cout << "Service call succeeded" << std::endl;
  }
  else
    std::cerr << "Service call failed" << std::endl;
}
