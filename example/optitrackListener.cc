/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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
#include "Optitrack.hh"

void usage()
{
  std::cerr << "Usage: optitrackListener <localIP> <threshold>" << std::endl
            << std::endl
            << "\t<localIP>      IP address used to communicate with OptiTrack."
            << std::endl
            << "\t<threshold>    Minimum distance (m) to consider that the "
            << "body has moved." << std::endl << std::endl;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Sanity check: Verify that we have expected command line args.
  if (argc != 3)
  {
    usage();
    return -1;
  }

  std::string localIP = std::string(argv[1]);
  float threshold = std::stof(std::string(argv[2]));
  Optitrack optitrack(localIP, threshold);

  // Blocking call to listen OptiTrack updates.
  optitrack.Receive();

  return 0;
}
