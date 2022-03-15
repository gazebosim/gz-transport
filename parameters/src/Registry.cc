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

using namespace ignition;
using namespace transport;
using namespace parameters;

using ParametersMapT = std::unordered_map<std::string, ParametersRegistry::ParameterValue>;

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

  ignition::transport::Node node;
  std::mutex parametersMapMutex;
  ParametersMapT parametersMap;
};

ParametersRegistry::ParametersRegistry(
  std::string _parametersServicesNamespace)
  : dataPtr{std::make_unique<ParametersRegistryPrivate>()}
{
  std::string getParameterSrvName{_parametersServicesNamespace + "/get_parameter"};
  this->dataPtr->node.Advertise(getParameterSrvName,
    &ParametersRegistryPrivate::GetParameter, this->dataPtr.get());

  std::string listParametersSrvName{_parametersServicesNamespace + "/list_parameters"};
  this->dataPtr->node.Advertise(listParametersSrvName,
    &ParametersRegistryPrivate::ListParameters, this->dataPtr.get());

  std::string setParameterSrvName{_parametersServicesNamespace + "/set_parameter"};
  this->dataPtr->node.Advertise(setParameterSrvName,
    &ParametersRegistryPrivate::SetParameter, this->dataPtr.get());

  std::string setParameterSrvName{_parametersServicesNamespace + "/declare_parameter"};
  this->dataPtr->node.Advertise(setParameterSrvName,
    &ParametersRegistryPrivate::DeclareParameter, this->dataPtr.get());
}

bool ParametersRegistryPrivate::GetParameter(const msgs::ParameterName &_req,
  msgs::ParameterValue &_res)
{
  const auto & param_name = _req.name();
  std::string protoType;
  std::ostringstream oss;
  {
    std::lock_guard guard{this->parametersMapMutex};
    auto it = this->registry.find(param_name);
    if (it == this->registry.end()) {
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
  // TODO(ivanpauno): Maybe the response should only include parameter names (?)
  // Maybe only names and types (?)
  // Including the component key doesn't seem to matter much, though it's also not wrong.
  {
    std::lock_guard guard{this->registryMutex};
    for (const auto & paramPair: this->registry) {
      auto * decl = _res.add_parameter_declarations();
      decl->set_name(paramPair.first);
      const auto & cmpKey = paramPair.second.componentKey;
      decl->set_type(paramPair.second.protoType);
      decl->set_component_type_id(cmpKey.first);
      decl->set_component_id(cmpKey.second);
    }
  }
  return true;
}

bool ParametersRegistryPrivate::SetParameter(const msgs::Parameter &_req, msgs::Boolean &_res)
{
  const auto & param_name = _req.name();
  ComponentKey key;
  Entity entity;
  std::string protoType;
  {
    std::lock_guard guard{this->registryMutex};
    auto it = this->registry.find(param_name);
    if (it == this->registry.end()) {
      return false;
    }
    key = it->second.componentKey;
    entity = it->second.entity;
    protoType = it->second.protoType;
  }
  if (protoType != _req.type()) {
    // parameter type doesn't match
    return false;
  }
  auto * component = this->ecm->Component<components::BaseComponent>(key);
  if (!component) {
    // component was removed
    // TODO(ivanpauno): Add a way to underclare a parameter
    return false;
  }
  std::istringstream iss{_req.value()};
  component->Deserialize(iss);
  this->ecm->SetChanged(entity, key.first);
  _res.set_data(true);
  // TODO(ivanpauno): Without this, the call is timing out for some reason.
  return true;
}

//   void
//   DeclareParameter(std::string parameterName, const google::protobuf::Message * initialValue);

//   std::unique_ptr<google::protobuf::Message>
//   GetParameter(std::string parameterName);
  
//   void
//   SetParameter(std::string parameterName, const google::protobuf::Message * value);
  
//   template<typename ProtoMsgT>
//   void
//   DeclareParameter(std::string parameterName, ProtoMsgT initialValue);
  
//   template<typename ProtoMsgT>
//   ProtoMsgT
//   GetParameter(std::string parameterName);
  
//   template<typename ProtoMsgT>
//   void
//   SetParameter(std::string parameterName, ProtoMsgT value);
// };
