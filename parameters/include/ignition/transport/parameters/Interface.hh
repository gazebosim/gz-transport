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

#ifndef IGNITION_TRANSPORT_PARAMETERS_INTERFACE_HH_
#define IGNITION_TRANSPORT_PARAMETERS_INTERFACE_HH_

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

      /// \brief Common interface, implemented by ParametersRegistry (local updates)
      ///   and by ParametersClients (remote requests).
      class ParametersInterface
      {
        /// Default virtual destructor.
        public: virtual ~ParametersInterface() = default;

        /// \brief Declare a new parameter.
        /// \param[in] _parameterName Name of the parameter to be declared.
        /// \param[in] _msg Protobuf message to be used as the initial
        ///   parameter value.
        /// \throw ParameterAlreadyDeclaredException if a parameter with the
        ///   same name was declared before.
        public: virtual void DeclareParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _msg) = 0;

        /// \brief Request the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be requested.
        /// \return The value of the parameter (a protobuf msg).
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw std::runtime_error if an unexpected error happens.
        public: virtual std::unique_ptr<google::protobuf::Message> Parameter(
          const std::string & _parameterName) const = 0;
        
        /// \brief Request the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be requested.
        /// \param[out] _parameter Output were the parameter value will be set.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if the type of `_parameter` does not match
        ///   the type of the parameter when it was declared.
        /// \throw std::runtime_error if an unexpected error happens.
        public: virtual void Parameter(
          const std::string & _parameterName, google::protobuf::Message & _parameter) const = 0;

        /// \brief Set the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be set.
        /// \param[in] _msg Protobuf message to be used as the parameter value.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if the type does not match
        ///   the type of the parameter when it was declared.
        public: virtual void SetParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _msg) = 0;

        /// \brief List all existing parameters.
        /// \return The name and types of existing parameters.
        public: virtual ignition::msgs::ParameterDeclarations ListParameters() const = 0;

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
        ProtoMsgT Parameter(const std::string & _parameterName) const
        {
          ProtoMsgT ret;
          this->Parameter(_parameterName, ret);
          return ret;
        }
      };
      }
    }
  }
}

#endif
