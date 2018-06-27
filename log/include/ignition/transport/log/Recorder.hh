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
#ifndef IGNITION_TRANSPORT_LOG_RECORDER_HH_
#define IGNITION_TRANSPORT_LOG_RECORDER_HH_

#include <cstdint>
#include <memory>
#include <regex>
#include <string>

#include <ignition/transport/config.hh>
#include <ignition/transport/log/Export.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
      //
      enum class RecorderError : int64_t
      {
        SUCCESS = 0,
        FAILED_TO_OPEN = -1,
        FAILED_TO_SUBSCRIBE = -2,
        ALREADY_RECORDING = -3,
        INVALID_TOPIC = -4,
        TOPIC_NOT_FOUND = -5,
      };

      /// \brief Records ignition transport topics
      /// This class makes it easy to record topics to a log file.
      /// Responsibilities: topic name matching, time received tracking,
      /// multiple thread safety, subscribing to topics
      class IGNITION_TRANSPORT_LOG_VISIBLE Recorder
      {
        /// \brief Default constructor
        public: Recorder();

        /// \brief move constructor
        /// \param[in] _old the instance being moved into this one
        public: Recorder(Recorder &&_old);  // NOLINT

        /// \brief destructor
        public: ~Recorder();

        /// \brief Begin recording topics
        /// \param[in] _file path to log file
        /// \return NO_ERROR if recording was successfully started. If the file
        /// already existed, this will return FAILED_TO_OPEN.
        public: RecorderError Start(const std::string &_file);

        /// \brief Stop recording topics
        public: void Stop();

        /// \brief Add a topic to be recorded (exact match only)
        /// \param[in] _topic The exact topic name
        /// \note This method attempts to subscribe to the topic immediately.
        ///       The subscription will be kept until this is destructed.
        /// \return NO_ERROR if the subscription was created.
        public: RecorderError AddTopic(const std::string &_topic);

        /// \brief Add a topic to be recorded (regex match)
        /// \param[in] _topic Pattern to match against topic names
        /// \note This method attempts to subscribe to topics immediately.
        ///       These subscriptions will be kept until this is destructed.
        ///       New topics that match the pattern will be added as they
        ///       appear, including while recording is active.
        /// \return number of topics subscribed or negative number on error
        public: int64_t AddTopic(const std::regex &_topic);

        public: std::string Filename() const;

        /// \internal Implementation of this class
        private: class Implementation;

        /// \internal Pointer to the implementation
        private: std::unique_ptr<Implementation> dataPtr;
      };
      }
    }
  }
}
#endif
