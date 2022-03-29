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

      class ParametersClient
      {
      public:
        ParametersClient(
          std::string _serverNamespace = "",
          unsigned int _timeoutMs = kDefaultTimeoutMs);

        std::unique_ptr<google::protobuf::Message>
        GetParameter(const std::string & _parameterName);

        void
        SetParameter(const std::string & _parameterName, const google::protobuf::Message & _msg);

        void
        DeclareParameter(const std::string & _parameterName, const google::protobuf::Message & _msg);

        ignition::msgs::ParameterDeclarations
        ListParameters();

      private:
        std::string serverNamespace;
        ignition::transport::Node node;
        unsigned int timeoutMs;

        constexpr static inline unsigned int kDefaultTimeoutMs = 5000;
      };
      }
    }
  }
}

#endif
