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

#ifndef IGNITION_TRANSPORT_PARAMETERS_CLIENT_HH_
#define IGNITION_TRANSPORT_PARAMETERS_CLIENT_HH_

#include <memory>
#include <string>

#include "google/protobuf/message.h"

#include "ignition/msgs/parameter_declarations.pb.h"

#include "ignition/transport/Node.hh"

namespace ignition
{
  namespace transport
  {
    namespace parameters
    {

      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {

      class ParametersClientPrivate;

      /// \brief Allow to get, set, declare or list parameters
      /// \brief in a remote registry.
      class ParametersClient
      {
      public:
        /// \brief Constructor.
        /// \param[in] _serverNamespace Namespace of the parameters registry
        ///    services. The client will send requests to:
        ///   * /${_serverNamespace}/get_parameter
        ///   * /${_serverNamespace}/list_parameters
        ///   * /${_serverNamespace}/set_parameter
        ///   * /${_serverNamespace}/declare_parameter
        /// \param[in] _timeoutMs Time to wait for the server to respond.
        ParametersClient(
          const std::string & _serverNamespace = "",
          unsigned int _timeoutMs = kDefaultTimeoutMs);

        /// \brief Destructor.
        public: ~ParametersClient();

        /// \brief No copy constructor.
        public: ParametersClient(const ParametersClient &) = delete;
        /// \brief No copy assignment.
        public: ParametersClient &
          operator=(const ParametersClient &) = delete;
        /// \brief Default move constructor.
        public: ParametersClient(ParametersClient &&) = default;
        /// \brief Default move assignment.
        public: ParametersClient &
          operator=(ParametersClient &&) = default;

        /// \brief Request the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be requested.
        /// \return The value of the parameter (a protobuf msg).
        std::unique_ptr<google::protobuf::Message>
        GetParameter(const std::string & _parameterName) const;

        /// \brief Set the value of a parameter.
        /// \param[in] _parameterName Name of the parameter to be set.
        /// \param[in] _msg Protobuf message to be used as the parameter value.
        void
        SetParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _msg) const;

        /// \brief Declare a new parameter.
        /// \param[in] _parameterName Name of the parameter to be declared.
        /// \param[in] _msg Protobuf message to be used as the initial
        ///   parameter value.
        void
        DeclareParameter(
          const std::string & _parameterName,
          const google::protobuf::Message & _msg) const;

        /// \brief List all parameters.
        /// \return Protobuf message with a list of all declared parameter
        ///   names and their types.
        ignition::msgs::ParameterDeclarations
        ListParameters() const;

      private:
        /// \brief Pointer to implementation.
        private: std::unique_ptr<ParametersClientPrivate> dataPtr;

        constexpr static inline unsigned int kDefaultTimeoutMs = 5000;
      };
      }
    }
  }
}

#endif
