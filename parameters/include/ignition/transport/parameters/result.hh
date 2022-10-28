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
        /// \brief Possible result types of the different parameters opeartions.
        ParameterResultType {
          Success,
          AlreadyDeclared,
          InvalidType,
          NotDeclared,
          ClientTimeout,
          Unexpected,
        };

        /// \brief The return type used in all falible parameters methods.
        class ParameterResult {
          /// \brief Construct.
          /// \param _resultType Type of result of the operation.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          explicit ParameterResult(ParameterResultType _resultType);

          /// \brief Construct.
          /// \param _resultType Type of result of the operation.
          /// \param _paramName Name of the related parameter.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          ParameterResult(
            ParameterResultType _resultType, const std::string & _paramName);

          /// \brief Construct.
          /// \param _resultType Type of result of the operation.
          /// \param _paramName Name of the related parameter.
          /// \param _paramType Type of the related parameter.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          ParameterResult(
            ParameterResultType _resultType,
            const std::string & _paramName,
            const std::string & _paramType);

          /// \brief  Return the result type.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          ParameterResultType ResultType() const;

          /// \brief  Return the related parameter name.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          const std::string & ParamName() const;

          /// \brief  Return the related parameter type.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          const std::string & ParamType() const;

          /// \brief Coercion to bool type.
          /// True if ParameterErrorType::Success, else False.
          public: IGNITION_TRANSPORT_PARAMETERS_VISIBLE
          explicit operator bool() const;

          private: ParameterResultType resultType;
          private: std::string paramName;
          private: std::string paramType;
        };

        /// \brief Stream operator, for debug output.
        IGNITION_TRANSPORT_PARAMETERS_VISIBLE
        std::ostream & operator<<(std::ostream &, const ParameterResult &);
      }
    }
  }
}

#endif
