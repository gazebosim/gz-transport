/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#include <cstdint>
#include <limits>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/AdvertiseOptionsPrivate.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
AdvertiseOptions::AdvertiseOptions()
  : dataPtr(new AdvertiseOptionsPrivate())
{
}

//////////////////////////////////////////////////
AdvertiseOptions::AdvertiseOptions(const AdvertiseOptions &_other)
  : dataPtr(new AdvertiseOptionsPrivate())
{
  (*this) = _other;
}

//////////////////////////////////////////////////
AdvertiseOptions::~AdvertiseOptions()
{
}

//////////////////////////////////////////////////
AdvertiseOptions &AdvertiseOptions::operator=(const AdvertiseOptions &_other)
{
  this->SetScope(_other.Scope());
  return *this;
}

//////////////////////////////////////////////////
//AdvertiseOptions::AdvertiseOptions(const AdvertiseOptions &_other)
//  : dataPtr(new AdvertiseOptionsPrivate())
//{
//  this->SetMsgsPerSec(_other.MsgsPerSec());
//}

//////////////////////////////////////////////////
const Scope_t &AdvertiseOptions::Scope() const
{
  return this->dataPtr->scope;
}

//////////////////////////////////////////////////
void AdvertiseOptions::SetScope(const Scope_t &_scope)
{
  this->dataPtr->scope = _scope;
}

//////////////////////////////////////////////////
const uint64_t AdvertiseOptions::kUnthrottled =
  std::numeric_limits<uint64_t>::max();

//////////////////////////////////////////////////
bool AdvertiseOptions::Throttled() const
{
  return this->MsgsPerSec() != kUnthrottled;
}

//////////////////////////////////////////////////
uint64_t AdvertiseOptions::MsgsPerSec() const
{
  return this->dataPtr->msgsPerSec;
}

//////////////////////////////////////////////////
void AdvertiseOptions::SetMsgsPerSec(const uint64_t _newMsgsPerSec)
{
  this->dataPtr->msgsPerSec = _newMsgsPerSec;
}
