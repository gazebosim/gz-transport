/*
 * Copyright (C) 2019 Open Source Robotics Foundation
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

#include "ignition/transport/config.hh"
#include "ignition/transport/Export.hh"

namespace ignition
{
  namespace transport
  {
    // Inline bracket to help doxygen filtering.
    inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
    //
    // Forward declarations.
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

      /// \brief Assignment operator.
      /// \param[in] _other The new DiscoveryOptions.
      /// \return A reference to this instance.
      public: DiscoveryOptions &operator=(const DiscoveryOptions &_other);

      /// \brief Get whether the verbose mode is enabled or not.
      /// \return True when verbose is enabled.
      public: bool Verbose() const;

      /// \brief Enable or disable the "verbose mode".
      /// \param[in] _verbose True for enabling verbose mode or false otherwise.
      public: void SetVerbose(const bool _verbose);

      /// \brief Get the list of relays.
      /// \return The list of relays.
      public: std::vector<std::string> Relays() const;

      /// \brief Set a new list of relays.
      /// \param[in] _newRelays A new set of relays.
      public: void SetRelays(const std::vector<std::string> &_newRelays);

#ifdef _WIN32
// Disable warning C4251 which is triggered by
// std::unique_ptr
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
      /// \internal
      /// \brief Pointer to private data.
      protected: std::unique_ptr<DiscoveryOptionsPrivate> dataPtr;
#ifdef _WIN32
#pragma warning(pop)
#endif
    };
    }
  }
}
#endif
