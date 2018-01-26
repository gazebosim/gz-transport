/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#ifndef IGNITION_TRANSPORT_LOG_DESCRIPTOR_HH_
#define IGNITION_TRANSPORT_LOG_DESCRIPTOR_HH_

#include <map>
#include <string>
#include <memory>

#include <ignition/transport/log/Export.hh>


namespace ignition
{
  namespace transport
  {
    namespace log
    {
      // Forward declaration
      class Log;

      //////////////////////////////////////////////////
      /// \brief The Descriptor class provides meta-information about what a log
      /// contains. This may be useful for determining QueryOptions or for
      /// generating a high-level overview of a Log's contents.
      class IGNITION_TRANSPORT_LOG_VISIBLE Descriptor
      {
        /// \brief A map from a name (e.g. topic name or message type name) to
        /// the id of a row in one of the database tables. (name -> id)
        public: using NameToId = std::map<std::string, int64_t>;

        /// \brief A map from a name to a map from a name to a row ID.
        /// (name -> name -> id)
        public: using NameToMap = std::map<std::string, NameToId>;

        /// \brief A topic in the database is uniquely identified by a pair of
        /// (topic name, message type).
        /// This function allows you to find the id of a topic by searching
        /// (topic name -> message type name -> ID).
        /// \return A map from topic names to a map of message types to row ids.
        /// \sa GetMsgTypesToTopicsToId()
        /// \sa QueryMsgTypesOfTopic()
        public: const NameToMap &GetTopicsToMsgTypesToId() const;

        /// \brief A topic in the database is uniquely identified by a pair of
        /// (topic name, message type).
        /// This function allows you to find the id of a topic by searching
        /// (message type name -> topic name -> ID).
        /// \return A map from message types to a map of topic names to row ids.
        /// \sa GetTopicsToMsgTypesToId()
        /// \sa QueryTopicsOfMsgType()
        public: const NameToMap &GetMsgTypesToTopicsToId() const;

        /// \brief Get message types published on the given name of a topic.
        /// \param[in] _topicName Name of the topic that you are interested in.
        /// \return A map from the message types published on _topicName to the
        /// id of a row in the topics table.
        /// If _topicName is not available in this log, this will return a
        /// nullptr.
        public: const NameToId *QueryMsgTypesOfTopic(
          const std::string &_topicName) const;

        /// \brief Get topic names that were uses to published a message type.
        /// \param[in] _msgType Name of the message type that you are interested
        /// in.
        /// \return A map from topic names which published _msgType to the id
        /// of a row in the topics table.
        /// If _msgType was never published in this log, this will return a
        /// nullptr.
        public: const NameToId *QueryTopicsOfMsgType(
          const std::string &_msgType) const;

        // The Log class is a friend so that it can construct a Descriptor
        friend class Log;

        /// \brief Destructor
        public: ~Descriptor();

        /// \brief The Descriptor class must only be created by calling
        /// Log::GetDescriptor().
        private: Descriptor();

        /// \internal Implementation for this class
        class Implementation;

        /// \internal Pointer to implementation
        private: std::unique_ptr<Implementation> dataPtr;
      };

    }
  }
}


#endif
