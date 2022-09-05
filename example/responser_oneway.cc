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
#include <string>
#include <ignition/msgs.hh>
#include <gz/transport.hh>

//////////////////////////////////////////////////
void srvOneway(const gz::msgs::StringMsg &_req)
{
  std::cout << "Request received: [" << _req.data() << "]" << std::endl;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Create a transport node.
  gz::transport::Node node;
  std::string service = "/oneway";

  // Advertise a oneway service.
  if (!node.Advertise(service, srvOneway))
  {
    std::cerr << "Error advertising service [" << service << "]" << std::endl;
    return -1;
  }

  // Zzzzzz.
  gz::transport::waitForShutdown();
}
