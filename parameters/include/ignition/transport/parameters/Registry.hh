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

#include "google/protobuf/message.h"

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

      class ParametersRegistry
      {
        public: struct ParameterValue
        {
          std::unique_ptr<google::protobuf::Message> msg;
          std::string protoType;
        };

        public: ParametersRegistry(std::string _parametersServicesNamespace);
      
        public: void DeclareParameter(
          const std::string & _parameterName,
          std::unique_ptr<google::protobuf::Message> _initialValue);
      
        public: ParameterValue GetParameter(
          const std::string & _parameterName);
        
        public: void SetParameter(
          const std::string & _parameterName,
          std::unique_ptr<google::protobuf::Message> _value);

        public: void SetParameter(
          const std::string & _parameterName,
          google::protobuf::Message & _value);

        public: template<typename ProtoMsgT>
        ProtoMsgT GetParameter(const std::string & _parameterName)
        {
          ProtoMsgT ret;
          this->WithParameter(
            _parameterName,
            [nameCStr = _parameterName.c_str() , &ret](google::protobuf::Message & _msg) {
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
