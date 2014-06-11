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

namespace ignition
{
  namespace transport
  {
    class TopicUtils
    {
      /// \brief Determines if a topic name is valid.
      /// \param[in] _topic Topic name to be checked.
      /// \return true if the topic name is valid.
      public: static bool IsValidTopic(const std::string &_topic,
                                       const std::string &_ns = "");

      /// \brief Get the full topic path given a namespace and a topic name.
      /// \param[in] _nameSpace Namespace.
      /// \param[in] _topic Topic name.
      /// \return The full path applying the topic name to the namespace.
      public: static std::string GetScopedName(const std::string &_nameSpace,
                                               const std::string &_topic);
    };
  }
}

#endif
