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

#include <ostream>
#include <sstream>

#include "ignition/transport/parameters/errors.hh"

using namespace ignition;
using namespace transport;
using namespace parameters;

//////////////////////////////////////////////////
std::ostream &
ignition::transport::parameters::operator<<(
  std::ostream & os, const ParameterResult & ret)
{
  std::ostringstream ss;
  switch (ret.ErrorType()) {
    case ParameterResultType::Success:
      ss << "parameter operation succeeded";
      break;
    case ParameterResultType::AlreadyDeclared:
      ss << "parameter already declared";
      break;
    case ParameterResultType::NotDeclared:
      ss << "parameter not declared";
      break;
    case ParameterResultType::InvalidType:
      ss << "parameter type is not valid";
      break;
    case ParameterResultType::ClientTimeout:
      ss << "parameter client timed out";
      break;
    case ParameterResultType::Unexpected:
    default:
      ss << "parameter operation unexpected error";
      break;
  }
  if (ret.ParamName() != "") {
    ss << ", parameter name [" << ret.ParamName() << "]";
  }
  if (ret.ParamType() != "") {
    ss << ", parameter type [" << ret.ParamType() << "]";
  }
  os << ss.str();
  return os;
}
