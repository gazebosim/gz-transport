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

#include <stdexcept>
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
        ParameterErrorType {
          NoError,
          AlreadyDeclared,
          InvalidType,
          NotDeclared,
          ClientTimeout,
          Unexpected,
        };

        class IGNITION_TRANSPORT_PARAMETERS_VISIBLE ParameterError {
          public: ParameterError(ParameterErrorType _errorType)
          : errorType(_errorType)
          {}
          public: ParameterError(ParameterErrorType _errorType, std::string _paramName)
          : errorType(_errorType), paramName{_paramName}
          {}
          public: ParameterError(
            ParameterErrorType _errorType, std::string _paramName, std::string _paramType)
          : errorType(_errorType), paramName{_paramName}, paramType{_paramType}
          {}

          public: ParameterErrorType ErrorType() const {return errorType;}
          public: const std::string & ParamName() const {return paramName;}
          public: const std::string & ParamType() const {return paramType;}

          public: explicit operator bool() const {return errorType == ParameterErrorType::NoError;}
          private: ParameterErrorType errorType;
          private: std::string paramName;
          private: std::string paramType;
        };

      // class IGNITION_TRANSPORT_PARAMETERS_VISIBLE
      // ParameterAlreadyDeclaredException : public std::runtime_error
      // {
      //   public: ParameterAlreadyDeclaredException(
      //     const char * _prefix , const char * _paramName)
      //   : std::runtime_error(
      //     std::string{_prefix} + ": parameter [" +
      //     _paramName + "] is already declared")
      //   {}
      // };

      // class IGNITION_TRANSPORT_PARAMETERS_VISIBLE ParameterInvalidTypeException
      // : public std::invalid_argument
      // {
      //   public: ParameterInvalidTypeException(
      //     const char * _prefix, const char * _paramName,
      //     const char * _expectedParamType, const char * _providedParamType)
      //   : std::invalid_argument(
      //     std::string{_prefix} + ": parameter [" +
      //     _paramName + "] is of type [" + _expectedParamType + "] but type [" +
      //     _providedParamType + "] was provided")
      //   {}

      //   public: ParameterInvalidTypeException(
      //     const char * _prefix, const char * _paramName,
      //     const char * _providedParamType)
      //   : std::invalid_argument(
      //     std::string{_prefix} + ": provided parameter type [" +
      //     _providedParamType + "] for parameter [" + _paramName +
      //     "] is invalid")
      //   {}
      // };

      // class IGNITION_TRANSPORT_PARAMETERS_VISIBLE ParameterNotDeclaredException
      // : public std::runtime_error
      // {
      //   public: ParameterNotDeclaredException(
      //     const char * _prefix , const char * _paramName)
      //   : std::runtime_error(
      //     std::string{_prefix} + ": parameter [" +
      //     _paramName + "] is not declared")
      //   {}
      // };
      }
    }
  }
}

#endif
