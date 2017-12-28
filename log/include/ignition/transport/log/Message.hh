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
#ifndef IGNITION_TRANSPORT_LOG_MESSAGE_HH_
#define IGNITION_TRANSPORT_LOG_MESSAGE_HH_

#include <chrono>
#include <memory>


#include <ignition/transport/log/Export.hh>
#include <ignition/common/Time.hh>


namespace ignition
{
  namespace transport
  {
    namespace log
    {
      /// \brief Forward Declarations
      class MessagePrivate;

      /// \brief Represents a message in a bag file
      class IGNITION_TRANSPORT_LOG_VISIBLE Message
      {
        /// \brief Default constructor
        public: Message();

        /// \brief Construct with data
        public: Message(const common::Time &_timeRecv,
            const void *_data, std::size_t _dataLen,
            const char *_type, std::size_t _typeLen,
            const char *_topic, std::size_t _topicLen);

        /// \brief Destructor
        public: ~Message();

        /// \brief Get the message data
        public: std::string Data() const;

        /// \brief Get the message type as a string
        public: std::string Type() const;

        /// \brief Get the Topic name as a string
        public: std::string Topic() const;

        public: const common::Time &TimeReceived() const;

        /// \brief Private Implementation Pointer
        private: std::unique_ptr<MessagePrivate> dataPtr;
      };
    }
  }
}
#endif
