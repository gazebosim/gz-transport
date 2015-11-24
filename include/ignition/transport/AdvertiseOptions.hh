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

#ifndef __IGN_TRANSPORT_ADVERTISEOPTIONS_HH_INCLUDED__
#define __IGN_TRANSPORT_ADVERTISEOPTIONS_HH_INCLUDED__

#include <memory>
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Publisher.hh"

namespace ignition
{
  namespace transport
  {
    class AdvertiseOptionsPrivate;

    /// \class AdvertiseOptions AdvertiseOptions.hh
    /// ignition/transport/AdvertiseOptions.hh
    /// \brief A class for customizing the publication options for a topic or
    /// service advertised.
    /// E.g.: Set the scope of a topic/service.
    class IGNITION_VISIBLE AdvertiseOptions
    {
      /// \brief Constructor.
      public: AdvertiseOptions();

      /// \brief Copy constructor.
      /// \param[in] _other AdvertiseOptions to copy.
      public: AdvertiseOptions(const AdvertiseOptions &_other);

      /// \brief Destructor.
      public: virtual ~AdvertiseOptions();

      /// \brief Assignment operator.
      /// \param[in] _other The new AdvertiseOptions.
      /// \return A reference to this instance.
      public: AdvertiseOptions& operator=(const AdvertiseOptions &_other);

      /// \brief Get the scope used in this topic/service.
      /// \return The scope.
      /// \sa SetScope.
      public: const Scope_t &Scope() const;

      /// \brief Set the scope of the topic or service.
      /// \param[in] _scope The new scope.
      /// \return True when operation succeed or false if the scope was invalid.
      /// \sa Scope.
      public: bool SetScope(const Scope_t &_scope);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::AdvertiseOptionsPrivate> dataPtr;
    };
  }
}
#endif
