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
      enum class PlaybackError : int
      {
        NO_ERROR = 0,
        FAILED_TO_OPEN = -1,
        FAILED_TO_ADVERTIZE = -2,
        ALREADY_PLAYING = -3,
        NO_MESSAGES = -4,
      };

      /// \brief Forward declaration
      class PlaybackPrivate;

      /// \brief Playbacks ignition transport topics
      /// This class makes it easy to play topics from a lot file
      /// Responsibilities: topic name matching, time keeping,
      /// multiple thread safety, publishing data to topics
      /// TODO: make it subscribe to newly discovered topics
      class IGNITION_TRANSPORT_LOG_VISIBLE Playback
      {
        /// \brief Default constructor
        public: Playback();

        /// \brief move constructor
        /// \param[in] _old the instance being moved into this one
        public: Playback(Playback &&_old);  // NOLINT

        /// \brief destructor
        public: ~Playback();

        /// \brief Begin playing messages
        /// \param[in] _file path to log file
        /// \return NO_ERROR if playing was successfully started
        public: PlaybackError Start(const std::string &_file);

        /// \brief Stop playing messages
        public: void Stop();

        /// \brief Private implementation
        private: std::unique_ptr<PlaybackPrivate> dataPtr;
      };
    }
  }
}
#endif
