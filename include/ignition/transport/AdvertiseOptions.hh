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

#ifndef IGN_TRANSPORT_ADVERTISEOPTIONS_HH_
#define IGN_TRANSPORT_ADVERTISEOPTIONS_HH_

#include <memory>

#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class AdvertiseMessageOptionsPrivate;
    class AdvertiseServiceOptionsPrivate;

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
    class IGNITION_TRANSPORT_VISIBLE AdvertiseOptions
    {
      /// \brief Constructor.
      public: AdvertiseOptions() = default;

      /// \brief Copy constructor.
      /// \param[in] _other AdvertiseOptions to copy.
      public: AdvertiseOptions(const AdvertiseOptions &_other);

      /// \brief Destructor.
      public: virtual ~AdvertiseOptions() = default;

      /// \brief Assignment operator.
      /// \param[in] _other The new AdvertiseOptions.
      /// \return A reference to this instance.
      public: AdvertiseOptions &operator=(const AdvertiseOptions &_other);

      /// \brief Equality operator. This function checks if the given
      /// AdverriseOptions has identical content to this object.
      /// \param[in] _opts The options to compare against.
      /// \return True if this object matches the provided object.
      public: bool operator==(const AdvertiseOptions &_opts) const;

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

      /// \brief Scope of the topic/service.
      protected: Scope_t scope = Scope_t::ALL;
    };

    /// \class AdvertiseMessageOptions AdvertiseOptions.hh
    /// ignition/transport/AdvertiseOptions.hh
    /// \brief A class for customizing the publication options for a topic
    /// advertised.
    /// E.g.: Set the rate of messages per second published.
    class IGNITION_TRANSPORT_VISIBLE AdvertiseMessageOptions
      : public AdvertiseOptions
    {
      /// \brief Constructor.
      public: AdvertiseMessageOptions();

      /// \brief Copy constructor.
      /// \param[in] _other AdvertiseMessageOptions to copy.
      public: AdvertiseMessageOptions(const AdvertiseMessageOptions &_other);

      /// \brief Destructor.
      public: virtual ~AdvertiseMessageOptions();

      /// \brief Assignment operator.
      /// \param[in] _other The new AdvertiseMessageOptions.
      /// \return A reference to this instance.
      public: AdvertiseMessageOptions &operator=(
        const AdvertiseMessageOptions &_other);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<AdvertiseMessageOptionsPrivate> dataPtr;
    };

    /// \class AdvertiseServiceOptions AdvertiseOptions.hh
    /// ignition/transport/AdvertiseOptions.hh
    /// \brief A class for customizing the publication options for a service
    /// advertised.
    class IGNITION_TRANSPORT_VISIBLE AdvertiseServiceOptions
      : public AdvertiseOptions
    {
      /// \brief Constructor.
      public: AdvertiseServiceOptions();

      /// \brief Copy constructor.
      /// \param[in] _other AdvertiseServiceOptions to copy.
      public: AdvertiseServiceOptions(const AdvertiseServiceOptions &_other);

      /// \brief Destructor.
      public: virtual ~AdvertiseServiceOptions();

      /// \brief Assignment operator.
      /// \param[in] _other The new AdvertiseServiceOptions.
      /// \return A reference to this instance.
      public: AdvertiseServiceOptions &operator=(
        const AdvertiseServiceOptions &_other);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<AdvertiseServiceOptionsPrivate> dataPtr;
    };
  }
}
#endif
