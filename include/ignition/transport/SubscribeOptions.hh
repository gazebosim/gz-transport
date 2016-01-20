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

#ifndef __IGN_TRANSPORT_SUBSCRIBEOPTIONS_HH_INCLUDED__
#define __IGN_TRANSPORT_SUBSCRIBEOPTIONS_HH_INCLUDED__

#include <memory>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    // Forward declare private data class.
    struct SubscribeOptionsPrivate;

    /// \class SubscribeOptions SubscribeOptions.hh
    /// ignition/transport/SubscribeOptions.hh
    /// \brief A class for customizing the subscription options for a topic
    /// advertised.
    /// E.g.: Enable the string conversion of a message.
    class IGNITION_VISIBLE SubscribeOptions
    {
      /// \brief Constructor.
      public: SubscribeOptions();

      /// \brief Copy constructor.
      /// \param[in] _other SubscribeOptions to copy.
      public: SubscribeOptions(const SubscribeOptions &_other);

      /// \brief Destructor.
      public: virtual ~SubscribeOptions();

      /// \brief Assignment operator.
      /// \param[in] _other The new SubscribeOptions.
      /// \return A reference to this instance.
      public: SubscribeOptions &operator=(const SubscribeOptions &_other);

      /// \brief Get the scope used in this topic/service.
      /// \return The scope.
      /// \sa SetScope.
      /// \sa Scope_t.
      public: bool TextMode() const;

      /// \brief Set the scope of the topic or service.
      /// \param[in] _scope The new scope.
      /// \sa Scope.
      /// \sa Scope_t.
      public: void SetTextMode(const bool _enabled);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<SubscribeOptionsPrivate> dataPtr;
    };
  }
}
#endif
