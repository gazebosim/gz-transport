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

#ifndef IGNITION_TRANSPORT_LOG_LOG_HH_
#define IGNITION_TRANSPORT_LOG_LOG_HH_

#include <ios>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <ignition/common/Time.hh>
#include <ignition/transport/log/Batch.hh>
#include <ignition/transport/log/QueryOptions.hh>
#include <ignition/transport/log/Descriptor.hh>
#include <ignition/transport/log/Export.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      const std::string SchemaLocationEnvVar = "IGN_TRANSPORT_LOG_SQL_PATH";

      /// \brief Interface to a log file
      class IGNITION_TRANSPORT_LOG_VISIBLE Log
      {
        /// \brief constructor
        public: Log();

        /// \brief move constructor
        /// \param[in] _old the instance being moved into this one
        public: Log(Log &&_old);  // NOLINT

        /// \brief destructor
        public: ~Log();

        /// \return true if a log is open
        public: bool Valid() const;

        /// \brief Get the schema version of the opened log
        /// \return the current version of the schema in the log file
        /// \return empty string if the log has not been opened
        public: std::string Version();

        /// \brief Open a log file
        /// \param[in] _file path to log file
        /// \param[in] _mode flag indicating read only or read/write
        ///   Can use (in or out)
        public: bool Open(const std::string &_file,
            std::ios_base::openmode _mode = std::ios_base::in);

        /// \brief Get a Descriptor for this log. The Descriptor will be
        /// generated the first time that this function is called after a new
        /// file has been opened.
        /// \return A Descriptor for this log, if a log file is currently
        /// loaded. If no log file is loaded, this returns a nullptr.
        public: const Descriptor *GetDescriptor() const;

        /// \brief Insert a message into the log file
        /// \param[in] _time Time the message was received
        /// \param[in] _topic Name of the topic the message was on
        /// \param[in] _type Name of the message type
        /// \param[in] _data pointer to a buffer containing the message data
        /// \param[in] _len number of bytes of data
        /// \return true if the message was successfully inserted
        public: bool InsertMessage(
            const common::Time &_time,
            const std::string &_topic, const std::string &_type,
            const void *_data, std::size_t _len);

        /// \brief Get all messages from the bag file
        /// \return an iterator through the messages
        public: Batch AllMessages();

        /// \brief Get messages belonging to the given topics
        /// \param[in] _topics the list of topics to query
        /// \return an iterator through the messages
        public: Batch QueryMessages(
            const std::unordered_set<std::string> &_topics);

        /// \brief Get messages according to the specified options. By default,
        /// it will query all messages over the entire time range of the log.
        public: Batch QueryMessages(
            const QueryOptions &_options = AllTopics());

        /// \internal Implementation for this class
        private: class Implementation;

        /// \brief Private implementation
        private: std::unique_ptr<Implementation> dataPtr;
      };
    }
  }
}
#endif
