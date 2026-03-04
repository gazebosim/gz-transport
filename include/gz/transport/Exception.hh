/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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
#ifndef GZ_TRANSPORT_EXCEPTION_HH_
#define GZ_TRANSPORT_EXCEPTION_HH_

#include <stdexcept>
#include <string>

#include "gz/transport/config.hh"
#include "gz/transport/Export.hh"

namespace gz::transport
{
  // Inline bracket to help doxygen filtering.
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE {
  //
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4275)
#endif
  /// \brief Exception class for gz-transport errors.
  class GZ_TRANSPORT_VISIBLE Exception : public std::runtime_error
  {
    /// \brief Inherit constructors from std::runtime_error.
    public: using std::runtime_error::runtime_error;
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  }
}
#endif
