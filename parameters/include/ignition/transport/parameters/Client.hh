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

#include "ignition/transport/Node.hh"

class ParametersClient
{
public:
 ParametersClient(ignition::transport::Node node, std::string defaultServerNamespace = "");
 
 // When serverNamespace == "", the default one is used.
 std::unique_ptr<google::protobuf::Message>
 GetParameter(std::string parameterName, std::string serverNamespace = "");
 
 void
 SetParameter(std::string parameterName, const google::protobuf::Message * value, std::string serverNamespace = "");
 
 template<typename ProtoMsgT>
 ProtoMsgT
 GetParameter(std::string parameterName, std::string serverNamespace = "");
 
 template<typename ProtoMsgT>
 void
 SetParameter(std::string parameterName, ProtoMsgT value, std::string serverNamespace = "");
};

#endif
