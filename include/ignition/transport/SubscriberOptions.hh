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

#ifndef __IGN_TRANSPORT_SUBSCRIBEROPTIONS_HH_INCLUDED__
#define __IGN_TRANSPORT_SUBSCRIBEROPTIONS_HH_INCLUDED__

#include <memory>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class SubscriberOptionsPrivate;

    /// \class SubscriberOptions SubscriberOptions.hh
    /// ignition/transport/SubscriberOptions.hh
    /// \brief This class provides different options for a subscription.
    class IGNITION_VISIBLE SubscriberOptions
    {
      /// \brief
      public: SubscriberOptions();

      /// \brief
      public: ~SubscriberOptions() = default;

      /// \brief
      public: void Debug(bool _newValue);

      /// \brief
      public: bool Debug();

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<SubscriberOptionsPrivate> dataPtr;
    };
  }
}

#endif
