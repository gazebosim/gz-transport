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

#include "ignition/transport/AdvertiseOptions.hh"

using namespace ignition;
using namespace transport;

namespace ignition
{
  namespace transport
  {
    /// \internal
    /// \brief Private data for AdvertiseMessageOptions class.
    class AdvertiseMessageOptionsPrivate
    {
      /// \brief Constructor.
      public: AdvertiseMessageOptionsPrivate() = default;

      /// \brief Destructor.
      public: virtual ~AdvertiseMessageOptionsPrivate() = default;
    };

    /// \internal
    /// \brief Private data for AdvertiseServiceOptions class.
    class AdvertiseServiceOptionsPrivate
    {
      /// \brief Constructor.
      public: AdvertiseServiceOptionsPrivate() = default;

      /// \brief Destructor.
      public: virtual ~AdvertiseServiceOptionsPrivate() = default;
    };
  }
}



//////////////////////////////////////////////////
AdvertiseOptions::AdvertiseOptions(const AdvertiseOptions &_other)
{
  (*this) = _other;
}

//////////////////////////////////////////////////
AdvertiseOptions &AdvertiseOptions::operator=(const AdvertiseOptions &_other)
{
  this->SetScope(_other.Scope());
  return *this;
}

//////////////////////////////////////////////////
bool AdvertiseOptions::operator==(const AdvertiseOptions &_opts) const
{
  return this->scope == _opts.Scope();
}

//////////////////////////////////////////////////
const Scope_t &AdvertiseOptions::Scope() const
{
  return this->scope;
}

//////////////////////////////////////////////////
void AdvertiseOptions::SetScope(const Scope_t &_scope)
{
  this->scope = _scope;
}

//////////////////////////////////////////////////
AdvertiseMessageOptions::AdvertiseMessageOptions()
  : AdvertiseOptions(),
    dataPtr(new AdvertiseMessageOptionsPrivate())
{
}

//////////////////////////////////////////////////
AdvertiseMessageOptions::AdvertiseMessageOptions(
  const AdvertiseMessageOptions &_other)
  : AdvertiseOptions()
{
  (*this) = _other;
}

//////////////////////////////////////////////////
AdvertiseMessageOptions::~AdvertiseMessageOptions()
{
}

//////////////////////////////////////////////////
AdvertiseMessageOptions &AdvertiseMessageOptions::operator=(
  const AdvertiseMessageOptions &_other)
{
  return *this;
}

//////////////////////////////////////////////////
AdvertiseServiceOptions::AdvertiseServiceOptions()
  : AdvertiseOptions(),
    dataPtr(new AdvertiseServiceOptionsPrivate())
{
}

//////////////////////////////////////////////////
AdvertiseServiceOptions::AdvertiseServiceOptions(
  const AdvertiseServiceOptions &_other)
  : AdvertiseOptions()
{
  (*this) = _other;
}

//////////////////////////////////////////////////
AdvertiseServiceOptions::~AdvertiseServiceOptions()
{
}

//////////////////////////////////////////////////
AdvertiseServiceOptions &AdvertiseServiceOptions::operator=(
  const AdvertiseServiceOptions &_other)
{
  return *this;
}
