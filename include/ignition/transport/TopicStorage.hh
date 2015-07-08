/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef __IGN_TRANSPORT_TOPICSTORAGE_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICSTORAGE_HH_INCLUDED__

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class TopicStorage TopicStorage.hh ignition/transport/TopicStorage.hh
    /// \brief Store address information about topics and provide convenient
    /// methods for adding new topics, removing them, etc.
    class IGNITION_VISIBLE TopicStorage
    {
      /// \brief Constructor.
      public: TopicStorage() = default;

      /// \brief Destructor.
      public: virtual ~TopicStorage() = default;

      /// \brief Add a new address associated to a given topic and node UUID.
      /// \param[in] _publisher New publisher.
      /// \return true if the new entry is added or false if not (because it
      /// was already stored).
      public: bool AddPublisher(const Publisher &_publisher);

      /// \brief Return if there is any address stored for the given topic.
      /// \param[in] _topic Topic name.
      /// \return True if there is at least one entry stored for the topic.
      public: bool HasTopic(const std::string &_topic);

      /// \brief Return if there is any publisher stored for the given topic and
      /// process UUID.
      /// \param[in] _topic Topic name.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \return True if there is at least one address stored for the topic and
      /// process UUID.
      public: bool HasAnyPublishers(const std::string &_topic,
                                    const std::string &_pUuid);

      /// \brief Return if the requested publisher's address is stored.
      /// \param[in] _addr Publisher's address requested
      /// \return true if the publisher's address is stored.
      public: bool HasPublisher(const std::string &_addr);

      /// \brief Get the address information for a given topic and node UUID.
      /// \param[in] _topic Topic name.
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \param[out] _publisher Publisher's information requested.
      /// \return true if a publisher is found for the given topic and UUID pair
      public: bool GetPublisher(const std::string &_topic,
                                const std::string &_pUuid,
                                const std::string &_nUuid,
                                Publisher &_publisher);

      /// \brief Get the map of publishers stored for a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _info Map of publishers requested.
      /// \return true if at least there is one publisher stored.
      public: bool GetPublishers(const std::string &_topic,
                                 std::map<std::string,
                                    std::vector<Publisher>> &_info);

      /// \brief Remove a publisher associated to a given topic and UUID pair.
      /// \param[in] _topic Topic name
      /// \param[in] _pUuid Process UUID of the publisher.
      /// \param[in] _nUuid Node UUID of the publisher.
      /// \return True when the publisher was removed or false otherwise.
      public: bool DelPublisherByNode(const std::string &_topic,
                                      const std::string &_pUuid,
                                      const std::string &_nUuid);

      /// \brief Remove all the publishers associated to a given process.
      /// \param[in] _pUuid Process' UUID of the publisher.
      /// \return True when at least one address was removed or false otherwise.
      public: bool DelPublishersByProc(const std::string &_pUuid);

      /// \brief Given a process UUID, the function returns the list of
      /// publishers contained in this process UUID with its address information
      /// \param _pUuid Process UUID.
      /// \param _pubs Map of publishers where the keys are the node UUIDs and
      /// the value is its address information.
      public: void GetPublishersByProc(const std::string &_pUuid,
                                   std::map<std::string,
                                      std::vector<Publisher>> &_pubs);

      /// \brief Get the list of topics currently stored.
      /// \param[out] _topics List of stored topics.
      public: void GetTopicList(std::vector<std::string> &_topics) const;

      /// \brief Print all the information for debugging purposes.
      public: void Print();

      // The keys are topics. The values are another map, where the key is
      // the process UUID and the value a vector of publishers.
      private: std::map<std::string,
                        std::map<std::string, std::vector<Publisher>>> data;
    };
  }
}

#endif
