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
#ifndef IGNITION_TRANSPORT_LOG_PLAYBACK_HH_
#define IGNITION_TRANSPORT_LOG_PLAYBACK_HH_

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
      enum class PlaybackError : int64_t
      {
        NO_ERROR = 0,
        FAILED_TO_OPEN = -1,
        FAILED_TO_ADVERTISE = -2,
        ALREADY_PLAYING = -3,
        NO_MESSAGES = -4,
        NO_SUCH_TOPIC = -5,
        NOT_PLAYING = -6,
      };

      /// \brief Forward declaration
      class PlaybackPrivate;

      /// \brief Playbacks ignition transport topics
      /// This class makes it easy to play topics from a log file
      /// Responsibilities: topic name matching, time keeping,
      /// multiple thread safety, publishing data to topics
      class IGNITION_TRANSPORT_LOG_VISIBLE Playback
      {
        /// \brief Constructor
        /// \param[in] _file path to log file
        public: explicit Playback(const std::string &_file);

        /// \brief move constructor
        /// \param[in] _old the instance being moved into this one
        public: Playback(Playback &&_old);  // NOLINT

        /// \brief destructor
        public: ~Playback();

        /// \brief Begin playing messages
        /// \return NO_ERROR if playing was successfully started
        public: PlaybackError Start();

        /// \brief Stop playing messages
        public: PlaybackError Stop();

        /// \brief Block until playback runs out of messages to publish
        public: void WaitUntilFinished();

        /// \brief Add a topic to be played back (exact match only)
        /// \param[in] _topic The exact topic name
        /// \note This method attempts to advertise the topic immediately.
        ///       The publisher will be kept until this is destructed.
        /// \return NO_ERROR if the subscription was created.
        public: PlaybackError AddTopic(const std::string &_topic);

        /// \brief Add a topic to be played back (regex match)
        /// \param[in] _topic Pattern to match against topic names
        /// \note This method attempts to advertise topics immediately.
        ///       These publishers will be kept until this is destructed.
        /// \return number of topics published or negative number on error
        public: int64_t AddTopic(const std::regex &_topic);

        /// \brief Private implementation
        private: std::unique_ptr<PlaybackPrivate> dataPtr;
      };
    }
  }
}
#endif
