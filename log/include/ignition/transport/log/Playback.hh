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

#include <chrono>
#include <memory>
#include <regex>
#include <string>

#include <ignition/transport/log/Export.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      // Forward declarations
      class PlaybackHandle;
      using PlaybackHandlePtr = std::unique_ptr<PlaybackHandle>;

      //////////////////////////////////////////////////
      /// \brief Initiates playback of ignition transport topics
      /// This class makes it easy to play topics from a log file
      ///
      /// Responsibilities: topic name matching and initiating the playback
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
        /// \param[in] _waitAfterAdvertising How long to wait before the
        /// publications begin after advertising the topics that will be played
        /// back.
        ///
        /// \note The topic discovery process will need some time before
        /// publishing begins, or else subscribers in other processes will miss
        /// the outgoing messages. The default value is recommended unless you
        /// are confident in the timing of your system.
        ///
        /// \return A handle for managing the playback of the log. You must hold
        /// onto this object in order for the playback to continue, or else it
        /// will be cut short.
        ///
        /// If an error prevents the playback from starting, this will return a
        /// nullptr.
        //
        // TODO(MXG): When we can use C++17, add a [[nodiscard]] attribute here.
        public: PlaybackHandlePtr Start(
          const std::chrono::nanoseconds &_waitAfterAdvertising =
            std::chrono::seconds(1)) const;

        /// \brief Check if this Playback object has a valid log to play back
        /// \return true if this has a valid log to play back, otherwise false.
        public: bool Valid() const;

        /// \brief Add a topic to be played back (exact match only)
        /// \param[in] _topic The exact topic name
        /// \note This method attempts to advertise the topic immediately.
        ///       The publisher will be kept until this is destructed.
        /// \return True if the topic exists in the log, false if it does not
        /// exist or if the log is not valid.
        public: bool AddTopic(const std::string &_topic);

        /// \brief Add a topic to be played back (regex match)
        /// \param[in] _topic Pattern to match against topic names
        /// \note This method attempts to advertise topics immediately.
        ///       These publishers will be kept until this is destructed.
        /// \return number of topics that will be published or -1 if this
        /// Playback object is not valid.
        public: int64_t AddTopic(const std::regex &_topic);

        /// \internal Implementation of this class
        private: class Implementation;

        /// \brief Private implementation
        private: std::unique_ptr<Implementation> dataPtr;
      };

      //////////////////////////////////////////////////
      /// \brief Handles the playback of ignition transport topics.
      /// This class allows you to manage a log playback once it has started.
      /// You must hang onto the PlaybackHandle or else the playback will end
      /// early.
      ///
      /// Responsibilities: time keeping, multiple thread safety, publishing
      /// data to topics, and stopping playback.
      class IGNITION_TRANSPORT_LOG_VISIBLE PlaybackHandle
      {
        /// \brief Stop playing messages
        public: void Stop();

        /// \brief Block until playback runs out of messages to publish
        public: void WaitUntilFinished();

        /// \brief Destructor
        public: ~PlaybackHandle();

        // Friendship
        friend class Playback;

        // Forward declaration of implementation class
        private: class Implementation;

        /// \brief Private constructor. This can only be called by Playback.
        private: PlaybackHandle(std::unique_ptr<Implementation> &&_internal);

        /// \internal Pointer to implementation class
        private: std::unique_ptr<Implementation> dataPtr;
      };
    }
  }
}
#endif
