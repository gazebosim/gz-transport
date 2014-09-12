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
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    /// \brief Base class for all commands.
    class IGNITION_VISIBLE Command // : public TCLAP::StdOutput
    {
      //virtual void failure(TCLAP::CmdLineInterface& c, TCLAP::ArgException& e);

      //virtual void usage(TCLAP::CmdLineInterface& c);

      //virtual void version(TCLAP::CmdLineInterface& c);

      /// \brief Execute an 'ign' command related with ignition-transport.
      public: void Execute(int argc, char **argv);
    };
  }
}

/// \brief External hook to use the 'ign' command from the outside of the
/// library.
extern "C" IGNITION_VISIBLE void execute(int argc, char **argv)
{
  ignition::transport::Command cmd;
  cmd.Execute(argc, argv);
}

#endif
