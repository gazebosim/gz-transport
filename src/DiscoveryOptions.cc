/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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
#include <vector>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/DiscoveryOptions.hh"

using namespace ignition;
using namespace transport;

namespace ignition
{
  namespace transport
  {
    /// \internal
    /// \brief Private data for DiscoveryOptions class.
    class DiscoveryOptionsPrivate
    {
      /// \brief Constructor.
      public: DiscoveryOptionsPrivate() = default;

      /// \brief Destructor.
      public: virtual ~DiscoveryOptionsPrivate() = default;

      /// \brief True for enabling debug.
      public: bool verbose = false;

      /// \brief Collection of discovery relays.
      public: std::vector<std::string> relays;
    };
  }
}

//////////////////////////////////////////////////
DiscoveryOptions::DiscoveryOptions()
  : dataPtr(new DiscoveryOptionsPrivate())
{
}

//////////////////////////////////////////////////
DiscoveryOptions::DiscoveryOptions(const DiscoveryOptions &_otherDiscoveryOpts)
  : dataPtr(new DiscoveryOptionsPrivate())
{
}

//////////////////////////////////////////////////
DiscoveryOptions::~DiscoveryOptions()
{
}

//////////////////////////////////////////////////
bool DiscoveryOptions::Verbose() const
{
  return this->dataPtr->verbose;
}

//////////////////////////////////////////////////
void DiscoveryOptions::SetVerbose(const bool _verbose)
{
  this->dataPtr->verbose = _verbose;
}

//////////////////////////////////////////////////
std::vector<std::string> DiscoveryOptions::Relays() const
{
  return this->dataPtr->relays;
}

//////////////////////////////////////////////////
void DiscoveryOptions::SetRelays(const std::vector<std::string> &_newRelays)
{
  this->dataPtr->relays = _newRelays;
}
