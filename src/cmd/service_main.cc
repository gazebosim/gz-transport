/*
 * Copyright (C) 2021 Open Source Robotics Foundation
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

#include <ignition/utils/cli/CLI.hpp>

#include "ign.hh"

#include <ignition/transport/config.hh>

//////////////////////////////////////////////////
/// \brief Enumeration of available commands
enum class ServiceCommand
{
  kNone,
  kServiceList,
  kServiceInfo,
  kServiceReq,
};

//////////////////////////////////////////////////
/// \brief Structure to hold all available service options
struct ServiceOptions
{
  /// \brief Command to execute
  ServiceCommand command{ServiceCommand::kNone};

  /// \brief Name of the service
  std::string service{""};

  /// \brief Data used with a service request
  std::string reqData{""};

  /// \brief Request type to use when requesting
  std::string reqType{""};

  /// \brief Response type to use when requesting
  std::string repType{""};

  /// \brief Timeout to use when requesting (in milliseconds)
  int timeout{-1};
};

//////////////////////////////////////////////////
/// \brief Callback fired when options are successfully parsed
void runServiceCommand(const ServiceOptions &_opt)
{
  switch(_opt.command)
  {
    case ServiceCommand::kServiceList:
      cmdServiceList();
      break;
    case ServiceCommand::kServiceInfo:
      cmdServiceInfo(_opt.service.c_str());
      break;
    case ServiceCommand::kServiceReq:
      cmdServiceReq(_opt.service.c_str(),
          _opt.reqType.c_str(), _opt.repType.c_str(),
          _opt.timeout, _opt.reqData.c_str());
      break;
    case ServiceCommand::kNone:
    default:
      // In the event that there is no command, display help
      throw CLI::CallForHelp();
      break;
  }
}

//////////////////////////////////////////////////
void addServiceFlags(CLI::App &_app)
{
  auto opt = std::make_shared<ServiceOptions>();

  auto serviceOpt = _app.add_option("-s,--service",
                                    opt->service, "Name of a service");
  auto reqTypeOpt = _app.add_option("--reqtype",
                                    opt->reqType, "Type of a request.");
  auto repTypeOpt = _app.add_option("--reptype",
                                    opt->repType, "Type of a response.");
  auto timeoutOpt = _app.add_option("--timeout",
                                    opt->timeout, "Timeout in milliseconds.");

  auto command = _app.add_option_group("command", "Command to be executed");
  command->add_flag_callback("-l,--list",
      [opt](){
        opt->command = ServiceCommand::kServiceList;
      }, "List available services");

  command->add_flag_callback("-i,--info",
      [opt](){
        opt->command = ServiceCommand::kServiceInfo;
      }, "Get information about a service")
    ->needs(serviceOpt);

  command->add_option_function<std::string>("-r,--req",
      [opt](const std::string &_reqData){
        opt->command = ServiceCommand::kServiceReq;
        opt->reqData = _reqData;
      }, "Perform a service request")
    ->needs(serviceOpt)
    ->needs(reqTypeOpt)
    ->needs(repTypeOpt)
    ->needs(timeoutOpt);

  _app.callback([opt](){runServiceCommand(*opt); });
}

//////////////////////////////////////////////////
int main(int argc, char** argv)
{
  CLI::App app{"Introspect Ignition services"};

  app.set_help_all_flag("--help-all", "Show all help");

  app.add_flag_callback("-v,--version", [](){
      std::cout << IGNITION_TRANSPORT_VERSION_FULL << std::endl;
      throw CLI::Success();
  });

  addServiceFlags(app);
  CLI11_PARSE(app, argc, argv);
}
