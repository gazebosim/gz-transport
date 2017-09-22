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

#ifndef IGN_TRANSPORT_DISCOVERYOPTIONS_HH_
#define IGN_TRANSPORT_DISCOVERYOPTIONS_HH_

#include <memory>
#include <string>
#include <vector>

#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class DiscoveryOptionsPrivate;

    /// \class DiscoveryOptions DiscoveryOptions.hh
    /// ignition/transport/DiscoveryOptions.hh
    /// \brief A class to provide different options for discovery.
    class IGNITION_TRANSPORT_VISIBLE DiscoveryOptions
    {
      /// \brief Default constructor.
      public: DiscoveryOptions();

      /// \brief Copy constructor.
      /// \param[in] _otherDiscoveryOpts DiscoveryOptions to copy.
      public: DiscoveryOptions(const DiscoveryOptions &_otherDiscoveryOpts);

      /// \brief Destructor.
      public: virtual ~DiscoveryOptions();

      /// \brief Get whether the verbose mode is enabled or not.
      /// \return True when verbose is enabled.
      public: bool Verbose() const;

      /// \brief Set the discovery port.
      /// \param[in] _port The new port.
      public: void SetVerbose(const bool _verbose);

      /// \brief Get the list of relays.
      /// \return The list of relays.
      public: std::vector<std::string> Relays() const;

      /// \brief Set a new list of relays.
      /// \param[in] _newRelays A new set of relays.
      public: void SetRelays(const std::vector<std::string> &_newRelays);

      /// \internal
      /// \brief Pointer to private data.
      private: std::unique_ptr<DiscoveryOptionsPrivate> dataPtr;
    };
  }
}
#endif
