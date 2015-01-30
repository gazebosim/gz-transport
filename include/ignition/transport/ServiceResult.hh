/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifndef __IGN_TRANSPORT_SRVRESULT_HH_INCLUDED__
#define __IGN_TRANSPORT_SRVRESULT_HH_INCLUDED__

#include <memory>
#include <string>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class ServiceResultPrivate;

    /// \def Result_t This strongly typed enum defines the different options for
    /// the result of a service request.
    /// * Pending:   The service request is still in transit. The requester
    ///              should not use the response parameter.
    /// * Success:   The service request was succesfully executed.
    /// * Fail:      The service request was received but failed. The requester
    ///              should not use the response parameter.
    /// * Exception: The service request was received but generated an
    ///              exception. The exception information will be available and
    ///              could be consulted.
    enum class Result_t {Pending, Success, Fail, Exception};

    /// \class ServiceResult ServiceResult.hh
    /// ignition/transport/ServiceResult.hh
    /// \brief Store information about the result of a service call. The result
    /// can be Success, Fail or Exeption.
    class IGNITION_VISIBLE ServiceResult
    {
      /// \brief Class constructor.
      public: ServiceResult();

      /// \brief Default class destructor.
      public: virtual ~ServiceResult();

      /// \brief Copy constructor.
      public: ServiceResult(const ServiceResult &_res);

      /// \brief Copy assignment operator.
      public: ServiceResult& operator=(ServiceResult _other);

      /// \brief Get the return code stored in this object.
      /// \return The return code.
      public: Result_t ReturnCode() const;

      /// \brief Get the exception message.
      /// \return The exception message.
      public: std::string ExceptionMsg() const;

      /// \brief Set the return code.
      /// \param[in] _code New return code.
      public: void ReturnCode(const Result_t &_code);

      /// \brief Set the exception message.
      /// \param[in] _msg New exception message.
      public: void ExceptionMsg(const std::string &_msg);

      /// \brief Return if the ServiceResult returned succesfully.
      /// \return True when the result was Success or false otherwise.
      public: bool Succeed();

      /// \brief Return if the ServiceResult failed.
      /// \return True when the result was Fail or false otherwise.
      public: bool Failed();

      /// \brief Return if the ServiceResult triggered an exception.
      /// \return True when the result was Exception or false otherwise.
      public: bool Raised();

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<ServiceResultPrivate> dataPtr;
    };
  }
}

#endif
