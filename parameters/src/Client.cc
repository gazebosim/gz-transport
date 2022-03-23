/*
 * Copyright (C) 2022 Open Source Robotics Foundation
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

#include "ignition/transport/parameters/Client.hh"

#include <memory>
#include <string>
#include <sstream>

#include "ignition/msgs/boolean.pb.h"
#include "ignition/msgs/parameter_name.pb.h"
#include "ignition/msgs/parameter_value.pb.h"

#include "ignition/transport/parameters/exceptions.hh"

using namespace ignition;
using namespace transport;
using namespace parameters;


ParametersClient::ParametersClient(std::string _serverNamespace)
: serverNamespace{std::move(_serverNamespace)}
{}
 
std::unique_ptr<google::protobuf::Message>
ParametersClient::GetParameter(const std::string & _parameterName)
{
  constexpr unsigned int timeout{5000};

  bool result{false};
  const std::string service{serverNamespace + "/get_parameter"};

  msgs::ParameterName req;
  msgs::ParameterValue res;

  req.set_name(_parameterName);

  if (!node.Request(service, req, timeout, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::GetParameter(): request timed out"};
  }
  if (!result)
  {
    throw ParameterNotDeclaredException {
      "ParametersClient::GetParameter()", _parameterName.c_str()};
  }
  std::unique_ptr<google::protobuf::Message> ret =
    ignition::msgs::Factory::New(res.type());
  std::istringstream iss{res.value()};
  ret->ParseFromIstream(&iss);
  return ret;
}

void
ParametersClient::SetParameter(
  const std::string & _parameterName, const google::protobuf::Message & _msg)
{
  constexpr unsigned int timeout{5000};

  bool result{false};
  const std::string service{serverNamespace + "/set_parameter"};

  std::string protoType{"ign_msgs."};
  protoType += _msg.GetDescriptor()->name();

  msgs::Parameter req;
  msgs::Boolean res;

  req.set_name(_parameterName);
  req.set_type(std::move(protoType));

  std::ostringstream oss;
  _msg.SerializeToOstream(&oss);
  req.set_value(oss.str());

  if (!node.Request(service, req, timeout, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::SetParameter(): request timed out"};
  }
  if (!result)
  {
    throw ParameterNotDeclaredException {
      "ParametersClient::SetParameter()", _parameterName.c_str()};
  }
}

void
ParametersClient::DeclareParameter(
  const std::string & _parameterName, const google::protobuf::Message & _msg)
{
  constexpr unsigned int timeout{5000};

  bool result{false};
  const std::string service{serverNamespace + "/declare_parameter"};

  std::string protoType{"ign_msgs."};
  protoType += _msg.GetDescriptor()->name();

  msgs::Parameter req;
  msgs::Boolean res;

  req.set_name(_parameterName);
  req.set_type(std::move(protoType));

  std::ostringstream oss;
  _msg.SerializeToOstream(&oss);
  req.set_value(oss.str());

  if (!node.Request(service, req, timeout, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::DeclareParameter(): request timed out"};
  }
  if (!result)
  {
    throw ParameterAlreadyDeclaredException {
      "ParametersClient::DeclareParameter()", _parameterName.c_str()};
  }
}
