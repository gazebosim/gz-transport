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

#include <string>
#include "ignition/transport/ServiceResult.hh"
#include "ignition/transport/ServiceResultPrivate.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
ServiceResult::ServiceResult()
  : dataPtr(new ServiceResultPrivate())
{
}

//////////////////////////////////////////////////
ServiceResult::ServiceResult(const ServiceResult &_res)
  : dataPtr(new ServiceResultPrivate())
{
  this->ReturnCode(_res.ReturnCode());
  this->ExceptionMsg(_res.ExceptionMsg());
}

//////////////////////////////////////////////////
ServiceResult::~ServiceResult()
{
}

//////////////////////////////////////////////////
ServiceResult& ServiceResult::operator=(ServiceResult _other)
{
  this->ReturnCode(_other.ReturnCode());
  this->ExceptionMsg(_other.ExceptionMsg());

  return *this;
}

//////////////////////////////////////////////////
Result_t ServiceResult::ReturnCode() const
{
  return this->dataPtr->returnCode;
}

//////////////////////////////////////////////////
std::string ServiceResult::ExceptionMsg() const
{
  return this->dataPtr->exceptionMsg;
}

//////////////////////////////////////////////////
void ServiceResult::ReturnCode(const Result_t &_code)
{
  this->dataPtr->returnCode = _code;
}

//////////////////////////////////////////////////
void ServiceResult::ExceptionMsg(const std::string &_msg)
{
  this->dataPtr->exceptionMsg = _msg;
}

//////////////////////////////////////////////////
bool ServiceResult::Succeed()
{
  return this->ReturnCode() == Result_t::Success;
}

//////////////////////////////////////////////////
bool ServiceResult::Failed()
{
  return this->ReturnCode() == Result_t::Fail;
}

//////////////////////////////////////////////////
bool ServiceResult::Raised()
{
  return this->ReturnCode() == Result_t::Exception;
}
