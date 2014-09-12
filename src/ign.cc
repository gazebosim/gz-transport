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

#include <iostream>
#include <list>
#include <tclap/CmdLine.h>
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
/*void Command::failure(TCLAP::CmdLineInterface& c, TCLAP::ArgException& e)
{
  TCLAP::Arg arg = *(c.getArgList().front());
  //std::cout << arg << std::endl;

  std::cerr << ": " << std::endl
            << e.what() << std::endl;
  exit(1);
}

//////////////////////////////////////////////////
void Command::usage(TCLAP::CmdLineInterface& c)
{
  std::cout << "my usage message:" << std::endl;
  std::list<TCLAP::Arg*> args = c.getArgList();
  for (TCLAP::ArgListIterator it = args.begin(); it != args.end(); it++)
    std::cout << (*it)->longID()
              << "  (" << (*it)->getDescription() << ")" << std::endl;
}

//////////////////////////////////////////////////
void Command::version(TCLAP::CmdLineInterface& c)
{
  std::cout << "my version message: 0.1" << std::endl;
}*/

//////////////////////////////////////////////////
void Command::Execute(int argc, char **argv)
{
  /*std::cout << "ignition::transport::Execute()" << std::endl;
  std::cout << "Args:" << std::endl;
  for (int i = 0; i < argc; ++i)
    std::cout << argv[i] << std::endl;
  std::cout << "--" << std::endl;*/

  std::vector<std::string> allowedCommands = {"topic"};
  TCLAP::ValuesConstraint<std::string> allowedCmdVals(allowedCommands);
  std::vector<std::string> allowedSubcommands = {"list"};
  TCLAP::ValuesConstraint<std::string> allowedSubcmdVals(allowedSubcommands);

  try {
    TCLAP::CmdLine cmd("Tool for printing information about topics",
      ' ', "0.9");

    //cmd.setOutput(this);

    TCLAP::UnlabeledValueArg<std::string> commandLabel("commmand", "Command",
      true, "topic", &allowedCmdVals);
    cmd.add(commandLabel);

    TCLAP::UnlabeledValueArg<std::string> subcommandLabel("subcommand",
      "Subcommands", true, "list", &allowedSubcmdVals);
    cmd.add(subcommandLabel);

    // Parse the argv array.
    cmd.parse(argc, argv);

    Node node;

    std::string command = commandLabel.getValue();
    std::string subcommand = subcommandLabel.getValue();

    if (command == "topic")
    {
      if (subcommand == "list")
      {
        // Give the node some time to receive topic updates.
        sleep(1);
        std::vector<std::string> topics;
        node.GetTopicList(topics);

       for (auto topic : topics)
          std::cout << topic << std::endl;
      }
    }
  } catch (TCLAP::ArgException &e)
  {
    std::cerr << "Error:" << e.error() << " for arg " << e.argId() << std::endl;
  }
}
