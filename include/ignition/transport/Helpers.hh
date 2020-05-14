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

#ifndef IGN_TRANSPORT_HELPERS_HH_
#define IGN_TRANSPORT_HELPERS_HH_

#include <zmq.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "ignition/transport/config.hh"
#include "ignition/transport/Export.hh"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Avoid using deprecated message send/receive function when possible.
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 4, 0)
  #define IGN_ZMQ_POST_4_4_0
#endif

namespace ignition
{
  namespace transport
  {
    // Inline bracket to help doxygen filtering.
    inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
    //
    /// \brief Constant used when not interested in throttling.
    static const uint64_t kUnthrottled = std::numeric_limits<uint64_t>::max();

    /// \brief Find the environment variable '_name' and return its value.
    /// \param[in] _name Name of the environment variable.
    /// \param[out] _value Value if the variable was found.
    /// \return True if the variable was found or false otherwise.
    bool IGNITION_TRANSPORT_VISIBLE env(const std::string &_name,
                                        std::string &_value);

    /// \brief split at a one character delimiter to get a vector of something
    /// \param[in] _orig The string to split
    /// \param[in] _delim a character to split the string at
    /// \returns vector of split pieces of the string excluding the delimiter
    std::vector<std::string> IGNITION_TRANSPORT_VISIBLE split(
        const std::string &_orig,
        char _delim);

    // Use safer functions on Windows
    #ifdef _MSC_VER
      #define ign_strcat strcat_s
      #define ign_strcpy strcpy_s
      #define ign_sprintf sprintf_s
      #define ign_strdup _strdup
    #else
      #define ign_strcat std::strcat
      #define ign_strcpy std::strcpy
      #define ign_sprintf std::sprintf
      #define ign_strdup strdup
    #endif
  }
  }
}

// IGN_TRANSPORT_HELPERS_HH_
#endif
