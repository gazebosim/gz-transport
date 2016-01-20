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

namespace ignition
{
  namespace transport
  {
    class AdvertiseOptionsPrivate;

    /// \def Scope This strongly typed enum defines the different options for
    /// the scope of a topic/service.
    enum class Scope_t
    {
      /// \brief Topic/service only available to subscribers in the same
      /// process as the publisher.
      PROCESS,
      /// \brief Topic/service only available to subscribers in the same
      /// machine as the publisher.
      HOST,
      /// \brief Topic/service available to any subscriber (default scope).
      ALL
    };

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
      public: AdvertiseOptions &operator=(const AdvertiseOptions &_other);

      /// \brief Get the scope used in this topic/service.
      /// \return The scope.
      /// \sa SetScope.
      /// \sa Scope_t.
      public: const Scope_t &Scope() const;

      /// \brief Set the scope of the topic or service.
      /// \param[in] _scope The new scope.
      /// \sa Scope.
      /// \sa Scope_t.
      public: void SetScope(const Scope_t &_scope);

      /// \brief Get whether the plain TEXT advertise mode is enabled. By
      /// default all the topics are also advertised in plain TEXT mode.
      /// \return True if the plain TEXT mode advertise is enabled.
      /// \sa SetTextMode.
      public: bool TextMode() const;

      /// \brief Advertise also the plain TEXT version of the topic.
      /// \param[in] _enabled Advertise also the plain TEXT topic when true.
      /// \sa TextMode.
      public: void SetTextMode(const bool _enabled);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::AdvertiseOptionsPrivate> dataPtr;
    };
  }
}
#endif
