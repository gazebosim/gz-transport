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

#include <cstdint>
#include <cstring>
#include <iostream>

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
bool AdvertiseOptions::operator==(const AdvertiseOptions &_other) const
{
  return this->scope == _other.Scope();
}

//////////////////////////////////////////////////
bool AdvertiseOptions::operator!=(const AdvertiseOptions &_other) const
{
  return !(*this == _other);
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
size_t AdvertiseOptions::Pack(char *_buffer) const
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "AdvertiseOptions::Pack() error: NULL output buffer"
              << std::endl;
    return 0;
  }

  // Pack the scope.
  uint8_t intscope = static_cast<uint8_t>(this->Scope());
  memcpy(_buffer, &intscope, sizeof(intscope));

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseOptions::Unpack(const char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "AdvertiseOptions::Unpack() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  uint8_t intscope;
  memcpy(&intscope, _buffer, sizeof(intscope));
  this->scope = static_cast<Scope_t>(intscope);

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseOptions::MsgLength() const
{
  return sizeof(uint8_t);
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
  AdvertiseOptions::operator=(_other);
  return *this;
}

//////////////////////////////////////////////////
bool AdvertiseMessageOptions::operator==(
  const AdvertiseMessageOptions &_other) const
{
  return AdvertiseOptions::operator==(_other);
}

//////////////////////////////////////////////////
bool AdvertiseMessageOptions::operator!=(
  const AdvertiseMessageOptions &_other) const
{
  return !(*this == _other);
}

//////////////////////////////////////////////////
size_t AdvertiseMessageOptions::Pack(char *_buffer) const
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "AdvertiseMessageOptions::Pack() error: NULL output buffer"
              << std::endl;
    return 0;
  }

  // Pack the common part of any Advertise object.
  size_t len = AdvertiseOptions::Pack(_buffer);
  if (len == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseMessageOptions::Unpack(const char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "AdvertiseMessageOptions::Unpack() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  // Unpack the common part of any Advertise object.
  size_t len = AdvertiseOptions::Unpack(_buffer);
  if (len == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseMessageOptions::MsgLength() const
{
  return AdvertiseOptions::MsgLength();
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
  AdvertiseOptions::operator=(_other);
  return *this;
}

//////////////////////////////////////////////////
bool AdvertiseServiceOptions::operator==(
  const AdvertiseServiceOptions &_other) const
{
  return AdvertiseOptions::operator==(_other);
}

//////////////////////////////////////////////////
bool AdvertiseServiceOptions::operator!=(
  const AdvertiseServiceOptions &_other) const
{
  return !(*this == _other);
}

//////////////////////////////////////////////////
size_t AdvertiseServiceOptions::Pack(char *_buffer) const
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "AdvertiseServiceOptions::Pack() error: NULL output buffer"
              << std::endl;
    return 0;
  }

  // Pack the common part of any Advertise object.
  size_t len = AdvertiseOptions::Pack(_buffer);
  if (len == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseServiceOptions::Unpack(const char *_buffer)
{
  // null buffer.
  if (!_buffer)
  {
    std::cerr << "AdvertiseServiceOptions::Unpack() error: NULL input buffer"
              << std::endl;
    return 0;
  }

  // Unpack the common part of any Advertise object.
  size_t len = AdvertiseOptions::Unpack(_buffer);
  if (len == 0)
    return 0;

  return this->MsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseServiceOptions::MsgLength() const
{
  return AdvertiseOptions::MsgLength();
}
