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

#include "ignition/transport/parameters/Registry.hh"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "google/protobuf/message.h"

#include "ignition/transport/Node.hh"

#include <ignition/msgs/boolean.pb.h>
#include <ignition/msgs/parameter_declarations.pb.h>
#include <ignition/msgs/parameter_name.pb.h>
#include <ignition/msgs/parameter_value.pb.h>

#include <ignition/transport/parameters/exceptions.hh>


using namespace ignition;
using namespace transport;
using namespace parameters;

namespace
{
  struct ParameterValue
  {
    std::unique_ptr<google::protobuf::Message> msg;
    std::string protoType;
  };

  using ParametersMapT = std::unordered_map<
    std::string, ParameterValue>;
}

struct ignition::transport::parameters::ParametersRegistryPrivate
{
  /// \brief Get parameter service callback.
  /// \param[in] _req Request specifying the parameter name.
  /// \param[out] _res The value of the parameter.
  /// \return True if successful.
  bool GetParameter(const msgs::ParameterName &_req,
    msgs::ParameterValue &_res);

  /// \brief List parameter service callback.
  /// \param[in] _req unused.
  /// \param[out] _res List of available parameters.
  /// \return True if successful.
  bool ListParameters(const msgs::Empty &_req,
    msgs::ParameterDeclarations &_res);

  /// \brief Set parameter service callback.
  /// \param[in] _req Request specifying which parameter to set and its value.
  /// \param[out] _res Unused.
  /// \return True if successful.
  bool SetParameter(const msgs::Parameter &_req, msgs::Boolean &_res);

  /// \brief Declare parameter service callback.
  /// \param[in] _req Request specifying which parameter to be declared.
  /// \param[out] _res Unused.
  /// \return True if successful.
  bool DeclareParameter(const msgs::Parameter &_req, msgs::Boolean &_res);

  ignition::transport::Node node;
  std::mutex parametersMapMutex;
  ParametersMapT parametersMap;
};

ParametersRegistry::ParametersRegistry(
  const std::string &  _parametersServicesNamespace)
  : dataPtr{std::make_unique<ParametersRegistryPrivate>()}
{
  std::string getParameterSrvName{
    _parametersServicesNamespace + "/get_parameter"};
  this->dataPtr->node.Advertise(getParameterSrvName,
    &ParametersRegistryPrivate::GetParameter, this->dataPtr.get());

  std::string listParametersSrvName{
    _parametersServicesNamespace + "/list_parameters"};
  this->dataPtr->node.Advertise(listParametersSrvName,
    &ParametersRegistryPrivate::ListParameters, this->dataPtr.get());

  std::string setParameterSrvName{
    _parametersServicesNamespace + "/set_parameter"};
  this->dataPtr->node.Advertise(setParameterSrvName,
    &ParametersRegistryPrivate::SetParameter, this->dataPtr.get());

  std::string declareParameterSrvName{
    _parametersServicesNamespace + "/declare_parameter"};
  this->dataPtr->node.Advertise(declareParameterSrvName,
    &ParametersRegistryPrivate::DeclareParameter, this->dataPtr.get());
}

ParametersRegistry::~ParametersRegistry() = default;

bool ParametersRegistryPrivate::GetParameter(const msgs::ParameterName &_req,
  msgs::ParameterValue &_res)
{
  std::string protoType;
  std::ostringstream oss;
  {
    std::lock_guard guard{this->parametersMapMutex};
    auto it = this->parametersMap.find(_req.name());
    if (it == this->parametersMap.end()) {
      return false;
    }
    it->second.msg->SerializeToOstream(&oss);
    protoType = it->second.protoType;
  }
  _res.set_value(oss.str());
  _res.set_type(protoType);
  return true;
}

bool ParametersRegistryPrivate::ListParameters(const msgs::Empty &,
  msgs::ParameterDeclarations &_res)
{
  // TODO(ivanpauno): Maybe the response should only include parameter names,
  // maybe only names and types (?)
  // Including the component key doesn't seem to matter much,
  // though it's also not wrong.
  {
    std::lock_guard guard{this->parametersMapMutex};
    for (const auto & paramPair : this->parametersMap) {
      auto * decl = _res.add_parameter_declarations();
      decl->set_name(paramPair.first);
      decl->set_type(paramPair.second.protoType);
    }
  }
  return true;
}

bool ParametersRegistryPrivate::SetParameter(
  // cppcheck-suppress constParameter
  const msgs::Parameter &_req, msgs::Boolean &_res)
{
  (void)_res;
  const auto & paramName = _req.name();
  std::string protoType;
  {
    std::lock_guard guard{this->parametersMapMutex};
    auto it = this->parametersMap.find(paramName);
    if (it == this->parametersMap.end()) {
      return false;
    }
    protoType = it->second.protoType;
    if (protoType != _req.type()) {
      // parameter type doesn't match
      return false;
    }
    std::istringstream iss{_req.value()};
    it->second.msg->ParseFromIstream(&iss);
  }
  return true;
}

bool ParametersRegistryPrivate::DeclareParameter(
  // cppcheck-suppress constParameter
  const msgs::Parameter &_req, msgs::Boolean &_res)
{
  (void)_res;
  ParameterValue value;
  value.protoType = _req.type();
  value.msg = ignition::msgs::Factory::New(_req.type());
  if (!value.msg) {
    return false;
  }
  std::istringstream iss{_req.value()};
  value.msg->ParseFromIstream(&iss);
  std::lock_guard guard{this->parametersMapMutex};
  auto it_emplaced_pair = this->parametersMap.emplace(
    std::make_pair(_req.name(), std::move(value)));
  // return false when already declared
  return it_emplaced_pair.second;
}

void ParametersRegistry::DeclareParameter(
  const std::string & _parameterName,
  std::unique_ptr<google::protobuf::Message> _initialValue)
{
  if (!_initialValue) {
    throw std::invalid_argument{
      "ParametersRegistry::DeclareParameter(): `_parameterName` is nullptr"};
  }
  ParameterValue value;
  value.protoType = std::string{"ign_msgs."}
    + _initialValue->GetDescriptor()->name();
  value.msg = std::move(_initialValue);
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it_emplaced_pair = this->dataPtr->parametersMap.emplace(
    std::make_pair(_parameterName, std::move(value)));
  if (!it_emplaced_pair.second) {
    throw ParameterAlreadyDeclaredException{
      "ParametersRegistry::DeclareParameter()",
      _parameterName.c_str()};
  }
}

void ParametersRegistry::DeclareParameter(
  const std::string & _parameterName,
  const google::protobuf::Message & _msg)
{
  std::string protoType{"ign_msgs."};
  protoType += _msg.GetDescriptor()->name();
  auto new_msg = ignition::msgs::Factory::New(protoType);
  new_msg->CopyFrom(_msg);
  this->DeclareParameter(_parameterName, std::move(new_msg));
}

static auto GetParameterCommon(
  const ParametersRegistryPrivate & _dataPtr,
  const std::string & _parameterName)
{
  auto it = _dataPtr.parametersMap.find(_parameterName);
  if (it == _dataPtr.parametersMap.end()) {
    throw ParameterNotDeclaredException{
      "ParametersRegistry::Parameter()",
      _parameterName.c_str()};
  }
  return it;
}

std::unique_ptr<google::protobuf::Message>
ParametersRegistry::Parameter(const std::string & _parameterName) const
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = GetParameterCommon(*this->dataPtr, _parameterName);
  auto ret = ignition::msgs::Factory::New(it->second.protoType);
  if (!ret) {
    throw std::runtime_error{
      "ParametersRegistry::Parameter(): failed to create new message"
      " of type [" + it->second.protoType + "]"};
  }
  ret->CopyFrom(*it->second.msg);
  return ret;
}

void
ParametersRegistry::Parameter(
  const std::string & _parameterName,
  google::protobuf::Message & _parameter) const
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = GetParameterCommon(*this->dataPtr, _parameterName);
  std::string protoType{"ign_msgs."};
  protoType += _parameter.GetDescriptor()->name();
  if (protoType != it->second.protoType) {
    throw ParameterInvalidTypeException{
      "ParametersClient::Parameter()",
      _parameterName.c_str(),
      it->second.protoType.c_str(),
      protoType.c_str()};
  }
  _parameter.CopyFrom(*it->second.msg);
}

void
ParametersRegistry::SetParameter(
  const std::string & _parameterName,
  std::unique_ptr<google::protobuf::Message> _value)
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = this->dataPtr->parametersMap.find(_parameterName);
  if (it == this->dataPtr->parametersMap.end()) {
    throw ParameterNotDeclaredException{
      "ParametersRegistry::SetParameter()",
      _parameterName.c_str()};
  }
  // Validate the type matches before copying.
  if (it->second.msg->GetDescriptor() != _value->GetDescriptor()) {
    throw ParameterInvalidTypeException{
      "ParametersRegistry::SetParameter",
      _parameterName.c_str(),
      it->second.protoType.c_str(),
      (std::string{"ign_msgs."} + _value->GetDescriptor()->name()).c_str()};
  }
  it->second.msg = std::move(_value);
}

void
ParametersRegistry::SetParameter(
  const std::string & _parameterName,
  const google::protobuf::Message & _value)
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = this->dataPtr->parametersMap.find(_parameterName);
  if (it == this->dataPtr->parametersMap.end()) {
    throw ParameterNotDeclaredException{
      "ParametersRegistry::SetParameter()",
      _parameterName.c_str()};
  }
  // Validate the type matches before copying.
  if (it->second.msg->GetDescriptor() != _value.GetDescriptor()) {
    throw ParameterInvalidTypeException{
      "ParametersRegistry::SetParameter",
      _parameterName.c_str(),
      it->second.protoType.c_str(),
      (std::string{"ign_msgs."} + _value.GetDescriptor()->name()).c_str()};
  }
  it->second.msg->CopyFrom(_value);
}

ignition::msgs::ParameterDeclarations
ParametersRegistry::ListParameters() const
{
  ignition::msgs::ParameterDeclarations ret;
  ignition::msgs::Empty unused;

  dataPtr->ListParameters(unused, ret);
  return ret;
}
