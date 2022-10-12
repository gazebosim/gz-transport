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
#include <variant>

#include <google/protobuf/message.h>

#include <ignition/msgs/parameter_declarations.pb.h>

#include "ignition/transport/config.hh"
#include "ignition/transport/parameters/errors.hh"
#include "ignition/transport/parameters/Export.hh"

namespace ignition
{
  namespace transport
  {
    namespace parameters
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {

      // using ParameterResult = ::ignition::transport::parameters::ParameterResult;

      /// \brief Common interface, implemented by ParametersRegistry
      ///   (local updates) and by ParametersClients (remote requests).
      class IGNITION_TRANSPORT_PARAMETERS_VISIBLE ParametersInterface
      {
        /// Default virtual destructor.
        public: virtual ~ParametersInterface() = default;

        /// \brief Declare a new parameter.
        /// \param[in] _parameterName Name of the parameter to be declared.
        /// \param[in] _msg Protobuf message to be used as the initial
        ///   parameter value.
        /// \throw ParameterAlreadyDeclaredException if a parameter with the
        ///   same name was declared before.
        /// \throw ParameterInvalidTypeException (can only happen in client)
        ///   if the parameter server does not recognize the parameter type.
        public: virtual ParameterResult DeclareParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _msg) = 0;

        /// \brief Request the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be requested.
        /// \param[out] _parameter Output were the parameter value will be set.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if the type of `_parameter`
        ///    does not match the type of the parameter when it was declared.
        /// \throw std::runtime_error if an unexpected error happens.
        public: virtual ParameterResult Parameter(
          const std::string & _parameterName,
          google::protobuf::Message & _parameter) const = 0;

        /// \brief Set the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be set.
        /// \param[in] _msg Protobuf message to be used as the parameter value.
        /// \throw ParameterNotDeclaredException if a parameter of that name
        ///   was not declared before.
        /// \throw ParameterInvalidTypeException if the type does not match
        ///   the type of the parameter when it was declared.
        public: virtual ParameterResult SetParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _msg) = 0;

        /// \brief List all existing parameters.
        /// \return The name and types of existing parameters.
        public: virtual ignition::msgs::ParameterDeclarations
        ListParameters() const = 0;
      };
      }
    }
  }
}

#endif
