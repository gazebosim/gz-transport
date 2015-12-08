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

#ifndef __IGN_TRANSPORT_TOPICBLOCKERPRIVATE_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICBLOCKERPRIVATE_HH_INCLUDED__

#include <condition_variable>
#include <mutex>

namespace ignition
{
  namespace transport
  {
    /// \class TopicBlockerPrivate TopicBlockerPrivate.hh
    ///     ignition/transport/TopicBlockerPrivate.hh
    /// \brief Private data for TopicBlocker class.
    class TopicBlockerPrivate
    {
      /// \brief Constructor.
      public: TopicBlockerPrivate()
      {
      }

      /// \brief Destructor.
      public: virtual ~TopicBlockerPrivate() = default;

      /// \brief ToDo.
      public: std::condition_variable condition;

      /// \brief ToDo.
      public: std::mutex mutex;

      /// \brief ToDo.
      public: bool blocked = false;
    };
  }
}
#endif
