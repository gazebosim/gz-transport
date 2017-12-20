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

#include <memory>
#include <regex>
#include <string>

#include <ignition/common/Time.hh>
#include <ignition/transport/log/Export.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      enum class RecordError : int
      {
        NO_ERROR = 0,
        FAILED_TO_OPEN = -1,
        FAILED_TO_SUBSCRIBE = -2,
        ALREADY_RECORDING = -3,
        INVALID_TOPIC = -4,
        TOPIC_NOT_FOUND = -5,
      };

      /// \brief Forward declaration
      class RecordPrivate;

      /// \brief Records ignition transport topics
      /// This class makes it easy to record topics to a log file.
      /// Responsibilities: topic name matching, time received tracking,
      /// multiple thread safety, subscribing to topics
      /// TODO: make it subscribe to newly discovered topics
      class IGNITION_TRANSPORT_LOG_VISIBLE Record
      {
        /// \brief Default constructor
        public: Record();

        /// \brief move constructor
        /// \param[in] _old the instance being moved into this one
        public: Record(Record &&_old);  // NOLINT

        /// \brief destructor
        public: ~Record();

        /// \brief Begin recording topics
        /// \param[in] _file path to log file
        /// \return NO_ERROR if recording was successfully started
        public: RecordError Start(const std::string &_file);

        /// \brief Stop recording topics
        public: void Stop();

        /// \brief Add a topic to be recorded (exact match only)
        /// \param[in] _topic The exact topic name
        /// \note This method attempts to subscribe to the topic immediately.
        ///       The subscription will be kept until this is destructed.
        /// \return NO_ERROR if the subscription was created.
        public: RecordError AddTopic(const std::string &_topic);

        /// \brief Add a topic to be recorded (regex match)
        /// \param[in] _topic Pattern to match against topic names
        /// \note This method attempts to subscribe to topics immediately.
        ///       These subscriptions will be kept until this is destructed.
        /// \return number of topics subscribed or negative number on error
        public: int AddTopic(const std::regex &_topic);

        /// \brief Private implementation
        private: std::unique_ptr<RecordPrivate> dataPtr;
      };
    }
  }
}
#endif
