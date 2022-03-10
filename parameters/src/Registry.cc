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

#include "ignition/transport/log/Registry.hh"

#include <memory>
#include <string>

ParametersRegistry::ParametersRegistry(
    ignition::transport::Node _node,
    std::string _parametersServicesNamespace)
{
  
}
 
  void
  DeclareParameter(std::string parameterName, const google::protobuf::Message * initialValue);
 
  std::unique_ptr<google::protobuf::Message>
  GetParameter(std::string parameterName);
  
  void
  SetParameter(std::string parameterName, const google::protobuf::Message * value);
  
  template<typename ProtoMsgT>
  void
  DeclareParameter(std::string parameterName, ProtoMsgT initialValue);
  
  template<typename ProtoMsgT>
  ProtoMsgT
  GetParameter(std::string parameterName);
  
  template<typename ProtoMsgT>
  void
  SetParameter(std::string parameterName, ProtoMsgT value);
};

#endif
