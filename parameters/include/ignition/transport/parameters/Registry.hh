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

#ifndef IGNITION_TRANSPORT_PARAMETERS_REGISTRY_HH_
#define IGNITION_TRANSPORT_PARAMETERS_REGISTRY_HH_

#include <functional>
#include <memory>
#include <string>

#include <google/protobuf/message.h>

#include <ignition/msgs/parameter_declarations.pb.h>

#include "ignition/transport/config.hh"
#include "ignition/transport/parameters/exceptions.hh"

namespace ignition
{
  namespace transport
  {
    namespace parameters
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {

      struct ParametersRegistryPrivate;

      /// \brief Provides a parameter registry.
      /// Parameters can be declared, get or set in the registry.
      /// It also provides services, so the parameters can be get, set or
      /// listed from other processes.
      ///
      /// Provided services:
      /// * /${_parametersServicesNamespace}/get_parameter
      /// * /${_parametersServicesNamespace}/list_parameters
      /// * /${_parametersServicesNamespace}/set_parameter
      /// * /${_parametersServicesNamespace}/declare_parameter
      class ParametersRegistry
      {
        /// \brief Parameter message and its protobuf type name.
        public: struct ParameterValue
        {
          std::unique_ptr<google::protobuf::Message> msg;
          std::string protoType;
        };

        /// \brief Constructor.
        /// \param[in] _parametersServicesNamespace Namespace that will be used
        ///   in all the created services names.
        public: explicit ParametersRegistry(
          const std::string & _parametersServicesNamespace);

        /// \brief Destructor.
        public: ~ParametersRegistry();

        /// \brief No copy constructor.
        public: ParametersRegistry(const ParametersRegistry &) = delete;
        /// \brief No copy assignment.
        public: ParametersRegistry &
          operator=(const ParametersRegistry &) = delete;
        /// \brief Default move constructor.
        public: ParametersRegistry(ParametersRegistry &&) = default;
        /// \brief Default move assignment.
        public: ParametersRegistry &
          operator=(ParametersRegistry &&) = default;

        /// \brief Declare a new parameter.
        /// \param[in] _parameterName Name of the parameter.
        /// \param[in] _initialValue The initial value of the parameter.
        ///   The parameter type will be deduced from the type of the message.
        /// \throw std::invalid_argument if `_initialValue` is `nullptr`.
        /// \throw ParameterAlreadyDeclaredException if a parameter with the
        ///   same name was declared before.
        public: void DeclareParameter(
          const std::string & _parameterName,
          std::unique_ptr<google::protobuf::Message> _initialValue);

        /// \brief Get the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to get.
        /// \return The parameter value and its protobuf type.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw std::runtime_error if an unexpected error happens.
        public: ParameterValue GetParameter(
          const std::string & _parameterName) const;

        /// \brief Set the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to set.
        /// \param[in] _value The value of the parameter.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if the type does not match
        ///   the type of the parameter when it was declared.
        public: void SetParameter(
          const std::string & _parameterName,
          std::unique_ptr<google::protobuf::Message> _value);

        /// \brief Set the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to set.
        /// \param[in] _value The value of the parameter.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if the type does not match
        ///   the type of the parameter when it was declared.
        public: void SetParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _value);

        /// \brief List all existing parameters.
        /// \return The name and types of existing parameters.
        public: ignition::msgs::ParameterDeclarations ListParameters() const;

        /// \brief Get the value of a parameter.
        /// \tparam ProtoMsgT A protobuf message type, e.g.: ign::msgs::Boolean.
        /// \param[in] _parameterName Name of the parameter to get.
        /// \return The parameter value, as a protobuf message.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if ProtoMsgT does not match
        ///   the type of the parameter when it was declared.
        /// \throw std::runtime_error if an unexpected error happens.
        public: template<typename ProtoMsgT>
        ProtoMsgT GetParameter(const std::string & _parameterName) const
        {
          ProtoMsgT ret;
          this->WithParameter(
            _parameterName,
            [nameCStr = _parameterName.c_str() , &ret]
            (google::protobuf::Message & _msg) {
              if (_msg.GetDescriptor() != ret.GetDescriptor()) {
                throw ParameterInvalidTypeException{
                  "ParametersRegistry::GetParameter",
                  nameCStr,
                  _msg.GetDescriptor()->name().c_str(),
                  ret.GetDescriptor()->name().c_str()};
              }
              ret.CopyFrom(_msg);
            });
        }

        /// \brief Pointer to implementation.
        private: std::unique_ptr<ParametersRegistryPrivate> dataPtr;

        private: void WithParameter(
          const std::string & _parameterName,
          std::function<void(google::protobuf::Message &)> fn);
      };
      }
    }
  }
}

#endif
