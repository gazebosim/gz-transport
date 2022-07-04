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

#ifndef IGNITION_TRANSPORT_PARAMETERS_UTILS_HH_
#define IGNITION_TRANSPORT_PARAMETERS_UTILS_HH_

#include <optional>
#include <string>

#include "ignition/transport/config.hh"
#include <google/protobuf/any.pb.h>

namespace ignition
{
  namespace transport
  {
    namespace parameters
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {

      std::string add_ign_msgs_prefix(const std::string & ignType);

      std::optional<std::string> get_ign_type_from_any_proto(
        const google::protobuf::Any & any);
      }
    }
  }
}

#endif
