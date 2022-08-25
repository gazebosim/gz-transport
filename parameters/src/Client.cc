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
#include <sstream>
#include <string>

#include "ignition/msgs/boolean.pb.h"
#include <ignition/msgs/parameter_error.pb.h>
#include "ignition/msgs/parameter_name.pb.h"
#include "ignition/msgs/parameter_value.pb.h"

#include "ignition/transport/parameters/exceptions.hh"

#include "Utils.hh"

using namespace ignition;
using namespace transport;
using namespace parameters;

struct ignition::transport::parameters::ParametersClientPrivate
{
  ParametersClientPrivate(
    const std::string & _serverNamespace,
    unsigned int _timeoutMs)
  : serverNamespace{_serverNamespace},
    timeoutMs{_timeoutMs}
  {}
  std::string serverNamespace;
  mutable ignition::transport::Node node;
  unsigned int timeoutMs;
};

ParametersClient::~ParametersClient() = default;
ParametersClient::ParametersClient(ParametersClient &&) = default;
ParametersClient & ParametersClient::operator=(ParametersClient &&) = default;

ParametersClient::ParametersClient(
  const std::string & _serverNamespace,
  unsigned int _timeoutMs)
: dataPtr{std::make_unique<ParametersClientPrivate>(
    _serverNamespace,
    _timeoutMs)}
{}

static msgs::ParameterValue
GetParameterCommon(
  const ParametersClientPrivate & _dataPtr,
  const std::string & _parameterName)
{
  bool result{false};
  const std::string service{_dataPtr.serverNamespace + "/get_parameter"};

  msgs::ParameterName req;
  msgs::ParameterValue res;

  req.set_name(_parameterName);

  if (!_dataPtr.node.Request(service, req, _dataPtr.timeoutMs, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::Parameter(): request timed out"};
  }
  if (!result)
  {
    throw ParameterNotDeclaredException {
      "ParametersClient::Parameter()", _parameterName.c_str()};
  }
  return res;
}

std::unique_ptr<google::protobuf::Message>
ParametersClient::Parameter(const std::string & _parameterName) const
{
  auto res = GetParameterCommon(*this->dataPtr, _parameterName);
  auto ignTypeOpt = get_ign_type_from_any_proto(res.data());
  if (!ignTypeOpt) {
    throw std::runtime_error{
      (std::string{"unexpected serialized parameter type url ["} +
        res.data().type_url() + "]").c_str()};
  }
  auto ignType = add_ign_msgs_prefix(*ignTypeOpt);
  std::unique_ptr<google::protobuf::Message> ret =
    ignition::msgs::Factory::New(ignType);
  if (!ret) {
    throw std::runtime_error{
      std::string{"could not create parameter of type ["} + ignType + "]"};
  }
  if (!res.data().UnpackTo(ret.get())) {
    throw std::runtime_error{
      "failed to unpack parameter of type [" + ignType + "]"};
  }
  return ret;
}

void ParametersClient::Parameter(
  const std::string & _parameterName,
  google::protobuf::Message & _parameter) const
{
  auto res = GetParameterCommon(*this->dataPtr, _parameterName);
  auto ignTypeOpt = get_ign_type_from_any_proto(res.data());
  if (!ignTypeOpt) {
    throw std::runtime_error{
      (std::string{"unexpected serialized parameter type url ["} +
        res.data().type_url() + "]").c_str()};
  }
  auto ignType = *ignTypeOpt;
  if (ignType != _parameter.GetDescriptor()->name()) {
    throw ParameterInvalidTypeException{
      "ParametersClient::Parameter()",
      _parameterName.c_str(),
      ignType.c_str(),
      _parameter.GetDescriptor()->name().c_str()};
  }
  if (!res.data().UnpackTo(&_parameter)) {
    throw std::runtime_error{
      "failed to unpack parameter of type [" + ignType + "]"};
  }
}

void
ParametersClient::SetParameter(
  const std::string & _parameterName,
  const google::protobuf::Message & _msg)
{
  bool result{false};
  const std::string service{dataPtr->serverNamespace + "/set_parameter"};

  msgs::Parameter req;
  msgs::ParameterError res;

  req.set_name(_parameterName);
  req.mutable_value()->PackFrom(_msg);

  if (!dataPtr->node.Request(service, req, dataPtr->timeoutMs, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::SetParameter(): request timed out"};
  }
  if (!result)
  {
    throw std::runtime_error {
      "ParametersClient::SetParameter(): unexpected failure"};
  }
  if (res.data() == msgs::ParameterError::SUCCESS) {
    return;
  }
  if (res.data() == msgs::ParameterError::NOT_DECLARED) {
    throw ParameterNotDeclaredException{
      "ParametersClient::SetParameter()", _parameterName.c_str()};
  }
  if (res.data() == msgs::ParameterError::INVALID_TYPE) {
    throw ParameterInvalidTypeException{
      "ParametersClient::SetParameter()", _parameterName.c_str(),
      _msg.GetDescriptor()->name().c_str()};
  }
  throw std::runtime_error {
    "ParametersClient::SetParameter(): unexpected failure"};
}

void
ParametersClient::DeclareParameter(
  const std::string & _parameterName,
  const google::protobuf::Message & _msg)
{
  bool result{false};
  const std::string service{dataPtr->serverNamespace + "/declare_parameter"};

  msgs::Parameter req;
  msgs::ParameterError res;

  req.set_name(_parameterName);
  req.mutable_value()->PackFrom(_msg);

  if (!dataPtr->node.Request(service, req, dataPtr->timeoutMs, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::DeclareParameter(): request timed out"};
  }
  if (!result)
  {
    throw std::runtime_error {
      "ParametersClient::DeclareParameter(): unexpected failure"};
  }
  if (res.data() == msgs::ParameterError::SUCCESS) {
    return;
  }
  if (res.data() == msgs::ParameterError::ALREADY_DECLARED) {
    throw ParameterAlreadyDeclaredException{
      "ParametersClient::DeclareParameter()", _parameterName.c_str()};
  }
  if (res.data() == msgs::ParameterError::INVALID_TYPE) {
    throw ParameterInvalidTypeException{
      "ParametersClient::DeclareParameter()", _parameterName.c_str(),
      _msg.GetDescriptor()->name().c_str()};
  }
  throw std::runtime_error {
    "ParametersClient::DeclareParameter(): unexpected failure"};
}

msgs::ParameterDeclarations
ParametersClient::ListParameters() const
{
  bool result{false};
  const std::string service{dataPtr->serverNamespace + "/list_parameters"};

  msgs::Empty req;
  msgs::ParameterDeclarations res;

  if (!dataPtr->node.Request(service, req, dataPtr->timeoutMs, res, result))
  {
    throw std::runtime_error{
      "ParametersClient::ListParameters(): request timed out"};
  }
  if (!result)
  {
    throw std::runtime_error {
      "ParametersClient::ListParameters(): unexpected error"};
  }
  return res;
}
