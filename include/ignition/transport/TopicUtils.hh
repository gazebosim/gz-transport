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

#ifndef __IGN_TRANSPORT_TOPICUTILS_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICUTILS_HH_INCLUDED__

#include <string>

#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    /// \class TopicUtils TopicUtils.hh ignition/transport/TopicUtils.hh
    /// \brief This class provides different utilities related with topics.
    class IGNITION_VISIBLE TopicUtils
    {
      /// \brief Determines if a namespace is valid.
      /// \param[in] _ns Namespace to be checked.
      /// \return true if the namespace is valid.
      public: static bool IsValidNamespace(const std::string &_ns);

      /// \brief Determines if a partition is valid.
      /// The same rules to validate a topic name applies to a partition with
      /// the addition of the empty string, which is a valid partition (meaning
      /// no partition is used).
      /// \param[in] _partition Partition to be checked.
      /// \return true if the partition is valid.
      public: static bool IsValidPartition(const std::string &_partition);

      /// \brief Determines if a topic name is valid. A topic name is any
      /// non-empty alphanumeric string. The symbol '/' is also allowed as part
      /// of a topic name. The symbol '@' is not allowed in a topic name
      // because it is used as a partition delimitier.
      // Examples of valid topics: abc, /abc, /abc/de, /abc/de/
      /// \param[in] _topic Topic name to be checked.
      /// \return true if the topic name is valid.
      public: static bool IsValidTopic(const std::string &_topic);

      /// \brief Get the full topic path given a namespace and a topic name.
      /// \param[in] _partition Partition name.
      /// \param[in] _ns Namespace.
      /// \param[in] _topic Topic name.
      /// \param[out] _name Fully qualified topic name.
      /// \return True if the fully qualified name is valid
      /// (if partition, namespace and topic are correct).
      public: static bool FullyQualifiedName(const std::string &_partition,
                                             const std::string &_ns,
                                             const std::string &_topic,
                                             std::string &_name);
    };
  }
}

#endif
