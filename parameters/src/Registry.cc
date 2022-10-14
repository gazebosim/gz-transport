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
#include "google/protobuf/any.h"

#include "ignition/transport/Node.hh"

#include <ignition/msgs/boolean.pb.h>
#include <ignition/msgs/parameter_declarations.pb.h>
#include <ignition/msgs/parameter_error.pb.h>
#include <ignition/msgs/parameter_name.pb.h>
#include <ignition/msgs/parameter_value.pb.h>

#include <ignition/transport/parameters/result.hh>

#include "Utils.hh"

using namespace ignition;
using namespace transport;
using namespace parameters;

struct ignition::transport::parameters::ParametersRegistryPrivate
{
  using ParametersMapT = std::unordered_map<
    std::string, std::unique_ptr<google::protobuf::Message>>;

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
  bool SetParameter(const msgs::Parameter &_req, msgs::ParameterError &_res);

  /// \brief Declare parameter service callback.
  /// \param[in] _req Request specifying which parameter to be declared.
  /// \param[out] _res Unused.
  /// \return True if successful.
  bool DeclareParameter(
    const msgs::Parameter &_req, msgs::ParameterError &_res);

  ignition::transport::Node node;
  std::mutex parametersMapMutex;
  ParametersMapT parametersMap;
};

//////////////////////////////////////////////////
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

//////////////////////////////////////////////////
ParametersRegistry::~ParametersRegistry() = default;

//////////////////////////////////////////////////
ParametersRegistry::ParametersRegistry(ParametersRegistry &&) = default;

//////////////////////////////////////////////////
ParametersRegistry & ParametersRegistry::operator=(
  ParametersRegistry &&) = default;

//////////////////////////////////////////////////
bool ParametersRegistryPrivate::GetParameter(const msgs::ParameterName &_req,
  msgs::ParameterValue &_res)
{
  {
    std::lock_guard guard{this->parametersMapMutex};
    auto it = this->parametersMap.find(_req.name());
    if (it == this->parametersMap.end()) {
      return false;
    }
    _res.mutable_data()->PackFrom(*it->second, "ign_msgs");
  }
  return true;
}

//////////////////////////////////////////////////
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
      decl->set_type(addIgnMsgsPrefix(
        paramPair.second->GetDescriptor()->name()));
    }
  }
  return true;
}

//////////////////////////////////////////////////
bool ParametersRegistryPrivate::SetParameter(
  const msgs::Parameter &_req, msgs::ParameterError &_res)
{
  (void)_res;
  const auto & paramName = _req.name();
  {
    std::lock_guard guard{this->parametersMapMutex};
    auto it = this->parametersMap.find(paramName);
    if (it == this->parametersMap.end()) {
      _res.set_data(msgs::ParameterError::NOT_DECLARED);
      return true;
    }
    auto requestedIgnTypeOpt = getIgnTypeFromAnyProto(
      _req.value());
    if (!requestedIgnTypeOpt) {
      _res.set_data(msgs::ParameterError::INVALID_TYPE);
      return true;
    }
    auto requestedIgnType = *requestedIgnTypeOpt;
    if (it->second->GetDescriptor()->name() != requestedIgnType) {
      _res.set_data(msgs::ParameterError::INVALID_TYPE);
      return true;
    }
    if (!_req.value().UnpackTo(it->second.get())) {
      // unexpected error
      return false;
    }
  }
  return true;
}

//////////////////////////////////////////////////
bool ParametersRegistryPrivate::DeclareParameter(
  const msgs::Parameter &_req, msgs::ParameterError &_res)
{
  (void)_res;
  const auto & ignTypeOpt = getIgnTypeFromAnyProto(_req.value());
  if (!ignTypeOpt) {
    _res.set_data(msgs::ParameterError::INVALID_TYPE);
    return true;
  }
  auto ignType = addIgnMsgsPrefix(*ignTypeOpt);
  auto paramValue = ignition::msgs::Factory::New(ignType);
  if (!paramValue) {
    _res.set_data(msgs::ParameterError::INVALID_TYPE);
    return true;
  }
  if (!_req.value().UnpackTo(paramValue.get())) {
    // unexpected error
    return false;
  }
  std::lock_guard guard{this->parametersMapMutex};
  auto it_emplaced_pair = this->parametersMap.emplace(
    std::make_pair(_req.name(), std::move(paramValue)));
  if (!it_emplaced_pair.second) {
    _res.set_data(msgs::ParameterError::ALREADY_DECLARED);
  }
  return true;
}

//////////////////////////////////////////////////
ParameterResult
ParametersRegistry::DeclareParameter(
  const std::string & _parameterName,
  std::unique_ptr<google::protobuf::Message> _initialValue)
{
  if (!_initialValue) {
    throw std::invalid_argument{
      "ParametersRegistry::DeclareParameter(): `_parameterName` is nullptr"};
  }
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it_emplaced_pair = this->dataPtr->parametersMap.emplace(
    std::make_pair(_parameterName, std::move(_initialValue)));
  if (!it_emplaced_pair.second) {
    return ParameterResult{
      ParameterResultType::AlreadyDeclared,
      _parameterName};
  }
  return ParameterResult{ParameterResultType::Success};
}

//////////////////////////////////////////////////
ParameterResult
ParametersRegistry::DeclareParameter(
  const std::string & _parameterName,
  const google::protobuf::Message & _msg)
{
  auto protoType = addIgnMsgsPrefix(_msg.GetDescriptor()->name());
  auto newParam = ignition::msgs::Factory::New(protoType);
  if (!newParam) {
    return ParameterResult{
      ParameterResultType::Unexpected,
      _parameterName,
      protoType};
  }
  newParam->CopyFrom(_msg);
  this->DeclareParameter(_parameterName, std::move(newParam));
  return ParameterResult{ParameterResultType::Success};
}

//////////////////////////////////////////////////
ParameterResult
ParametersRegistry::Parameter(
  const std::string & _parameterName,
  google::protobuf::Message & _parameter) const
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = dataPtr->parametersMap.find(_parameterName);
  if (it == dataPtr->parametersMap.end()) {
    return ParameterResult{
      ParameterResultType::NotDeclared,
      _parameterName};
  }
  const auto & newProtoType = _parameter.GetDescriptor()->name();
  const auto & protoType = it->second->GetDescriptor()->name();
  if (newProtoType != protoType) {
    return ParameterResult{
      ParameterResultType::InvalidType,
      _parameterName,
      addIgnMsgsPrefix(protoType)};
  }
  _parameter.CopyFrom(*it->second);
  return ParameterResult{ParameterResultType::Success};
}

//////////////////////////////////////////////////
ParameterResult
ParametersRegistry::Parameter(
  const std::string & _parameterName,
  std::unique_ptr<google::protobuf::Message> & _parameter) const
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = dataPtr->parametersMap.find(_parameterName);
  if (it == dataPtr->parametersMap.end()) {
    return ParameterResult{
      ParameterResultType::NotDeclared,
      _parameterName};
  }
  const auto & protoType = it->second->GetDescriptor()->name();
  _parameter = ignition::msgs::Factory::New(protoType);
  if (!_parameter) {
    return ParameterResult{
      ParameterResultType::InvalidType,
      _parameterName,
      addIgnMsgsPrefix(protoType)};

  }
  _parameter->CopyFrom(*it->second);
  return ParameterResult{ParameterResultType::Success};
}

//////////////////////////////////////////////////
ParameterResult
ParametersRegistry::SetParameter(
  const std::string & _parameterName,
  std::unique_ptr<google::protobuf::Message> _value)
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = this->dataPtr->parametersMap.find(_parameterName);
  if (it == this->dataPtr->parametersMap.end()) {
    return ParameterResult{
      ParameterResultType::NotDeclared,
      _parameterName};
  }
  // Validate the type matches before copying.
  if (it->second->GetDescriptor() != _value->GetDescriptor()) {
    return ParameterResult{
      ParameterResultType::InvalidType,
      _parameterName,
      addIgnMsgsPrefix(it->second->GetDescriptor()->name())};
  }
  it->second = std::move(_value);
  return ParameterResult{ParameterResultType::Success};
}

//////////////////////////////////////////////////
ParameterResult
ParametersRegistry::SetParameter(
  const std::string & _parameterName,
  const google::protobuf::Message & _value)
{
  std::lock_guard guard{this->dataPtr->parametersMapMutex};
  auto it = this->dataPtr->parametersMap.find(_parameterName);
  if (it == this->dataPtr->parametersMap.end()) {
    return ParameterResult{
      ParameterResultType::NotDeclared,
      _parameterName};
  }
  // Validate the type matches before copying.
  if (it->second->GetDescriptor() != _value.GetDescriptor()) {
    return ParameterResult{
      ParameterResultType::InvalidType,
      _parameterName};
  }
  it->second->CopyFrom(_value);
  return ParameterResult{ParameterResultType::Success};
}

//////////////////////////////////////////////////
ignition::msgs::ParameterDeclarations
ParametersRegistry::ListParameters() const
{
  ignition::msgs::ParameterDeclarations ret;
  ignition::msgs::Empty unused;

  dataPtr->ListParameters(unused, ret);
  return ret;
}
