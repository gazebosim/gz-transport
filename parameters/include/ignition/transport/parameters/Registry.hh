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

#include <memory>
#include <string>

#include "google/protobuf/message.h"

#include <ignition/transport/config.hh>

namespace ignition
{
  namespace transport
  {
    namespace parameters
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {

      struct ParametersRegistryPrivate;

      class ParametersRegistry
      {
        public: struct ParameterValue
        {
          std::unique_ptr<google::protobuf::Message> msg;
          std::string protoType;
        };

        public: ParametersRegistry(std::string _parametersServicesNamespace);
      
        public: void DeclareParameter(
          std::string _parameterName,
          std::unique_ptr<google::protobuf::Message> _initialValue);
      
        public: ParameterValue GetParameter(
          std::string _parameterName);
        
        public: void SetParameter(
          std::string _parameterName,
          std::unique_ptr<google::protobuf::Message> _value);
        
        public: template<typename ProtoMsgT>
        void DeclareParameter(std::string _parameterName, ProtoMsgT _initialValue);
        
        public: template<typename ProtoMsgT>
        ProtoMsgT GetParameter(std::string _parameterName);
        
        public: template<typename ProtoMsgT>
        void SetParameter(std::string _parameterName, ProtoMsgT _value);

        private: std::unique_ptr<ParametersRegistryPrivate> dataPtr;
      };
      }
    }
  }
}

#endif
