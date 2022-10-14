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

#ifndef IGNITION_TRANSPORT_PARAMETERS_EXCEPTIONS_HH_
#define IGNITION_TRANSPORT_PARAMETERS_EXCEPTIONS_HH_

#include <ostream>
#include <string>

#include "ignition/transport/config.hh"
#include "ignition/transport/parameters/Export.hh"

namespace ignition
{
  namespace transport
  {
    namespace parameters
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
        enum class IGNITION_TRANSPORT_PARAMETERS_VISIBLE
        ParameterResultType {
          Success,
          AlreadyDeclared,
          InvalidType,
          NotDeclared,
          ClientTimeout,
          Unexpected,
        };

        class IGNITION_TRANSPORT_PARAMETERS_VISIBLE ParameterResult {
          public: explicit ParameterResult(ParameterResultType _errorType);
          public: ParameterResult(
            ParameterResultType _errorType, const std::string & _paramName);
          public: ParameterResult(
            ParameterResultType _errorType,
            const std::string & _paramName,
            const std::string & _paramType);

          public: ParameterResultType ErrorType() const;
          public: const std::string & ParamName() const;
          public: const std::string & ParamType() const;

          public: explicit operator bool() const;

          private: ParameterResultType errorType;
          private: std::string paramName;
          private: std::string paramType;
        };

        std::ostream & operator<<(std::ostream &, const ParameterResult &);
      }
    }
  }
}

#endif
