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

#ifndef __IGN_TRANSPORT_IGN_HH_INCLUDED__
#define __IGN_TRANSPORT_IGN_HH_INCLUDED__

#include <tclap/CmdLine.h>
#include <cstring>
#include <string>
#include "ignition/transport/config.hh"
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    /// \brief Base class for all commands.
    class IGNITION_VISIBLE Command
    {
      /// \brief Execute an 'ign' command related with ignition-transport.
      /// \param[in] _argc Number of arguments.
      /// \param[in] _argv Command line arguments.
      public: void Execute(int argc, char **argv);
    };
  }
}

/// \brief External hook to use the 'ign' command from the outside of the
/// library.
/// \param[in] _argc Number of arguments.
/// \param[in] _argv Command line arguments.
extern "C" IGNITION_VISIBLE void execute(int argc, char **argv)
{
  ignition::transport::Command cmd;
  cmd.Execute(argc, argv);
}

/// \brief External hook to read the library version.
/// \return C-string representing the version. Ex.: 0.1.2
extern "C" IGNITION_VISIBLE char *version()
{
  int majorVersion = IGNITION_TRANSPORT_MAJOR_VERSION;
  int minorVersion = IGNITION_TRANSPORT_MINOR_VERSION;
  int patchVersion = IGNITION_TRANSPORT_PATCH_VERSION;

  return strdup((std::to_string(majorVersion) + "." +
                 std::to_string(minorVersion) + "." +
                 std::to_string(patchVersion)).c_str());
}

#endif
