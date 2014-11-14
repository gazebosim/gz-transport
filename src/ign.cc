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

#include <tclap/CmdLine.h>
#include <chrono>
#include <iostream>
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
void Command::Execute(int argc, char **argv)
{
  // Used to constraint the list of commands.
  std::vector<std::string> allowedCommands = {"topic", "service"};
  TCLAP::ValuesConstraint<std::string> allowedCmdVals(allowedCommands);

  // Used to constraint the list of subcommands.
  std::vector<std::string> allowedSubcommands = {"list"};
  TCLAP::ValuesConstraint<std::string> allowedSubcmdVals(allowedSubcommands);

  try {
    TCLAP::CmdLine cmd("Tool for printing information about topics", ' ');

    TCLAP::UnlabeledValueArg<std::string> commandLabel("commmand", "Command",
      true, "topic", &allowedCmdVals, cmd);

    TCLAP::UnlabeledValueArg<std::string> subcommandLabel("subcommand",
      "Subcommands", true, "list", &allowedSubcmdVals, cmd);

    // Parse the argv array.
    cmd.parse(argc, argv);

    Node node;

    std::string command = commandLabel.getValue();
    std::string subcommand = subcommandLabel.getValue();

    if (command == "topic" || command == "service")
    {
      if (subcommand == "list")
      {
        // Give the node some time to receive topic updates.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::vector<std::string> topics;

        if (command == "topic")
          node.GetTopicList(topics);
        else
          node.GetServiceList(topics);

       for (auto topic : topics)
          std::cout << topic << std::endl;
      }
    }
  } catch (TCLAP::ArgException &e)
  {
    std::cerr << "Error:" << e.error() << " for arg " << e.argId() << std::endl;
  }
}
