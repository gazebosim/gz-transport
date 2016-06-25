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

#ifndef __IGN_TRANSPORT_TOPICWATCHER_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICWATCHER_HH_INCLUDED__

#include <memory>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class TopicWatcherPrivate;

    /// \class TopicWatcher TopicWatcher.hh ignition/transport/TopicWatcher.hh
    /// \brief ToDo.
    class IGNITION_TRANSPORT_VISIBLE TopicWatcher
    {
      /// \brief Constructor.
      public: TopicWatcher();

      /// \brief Destructor.
      public: virtual ~TopicWatcher();

      /// \brief ToDo.
      public: bool Wait(const unsigned int _timeout);

      /// \brief ToDo.
      public: void Release();

      /// \brief ToDo.
      public: bool Blocked() const;

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<TopicWatcherPrivate> dataPtr;
    };
  }
}
#endif