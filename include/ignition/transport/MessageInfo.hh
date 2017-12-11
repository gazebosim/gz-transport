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

#ifndef IGN_TRANSPORT_MESSAGEINFO_HH_
#define IGN_TRANSPORT_MESSAGEINFO_HH_

#include <memory>
#include <string>

#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class MessageInfoPrivate;

    /// \brief A class that provides information about the message received.
    class IGNITION_TRANSPORT_VISIBLE MessageInfo
    {
      /// \brief Default constructor.
      public: MessageInfo();

      /// \brief Explicit copy constructor (The copy constructor is deleted by
      /// default due to the use of std::unique_ptr member).
      public: MessageInfo(const MessageInfo &_other);

      /// \brief Destructor.
      public: ~MessageInfo();

      /// \brief Get the topic name associated to the message.
      /// \return The topic name.
      public: std::string Topic() const;

      /// \brief Set the topic name associated to the message.
      public: void SetTopic(const std::string &_topic);

      /// \internal
      /// \brief Pointer to private data.
      protected: std::unique_ptr<MessageInfoPrivate> dataPtr;
    };
  }
}
#endif
