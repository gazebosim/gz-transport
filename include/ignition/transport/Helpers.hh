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

#ifndef __IGN_TRANSPORT_HELPERS_HH_INCLUDED__
#define __IGN_TRANSPORT_HELPERS_HH_INCLUDED__

#include <cstdio>
#include <string>
#include <vector>

namespace ignition
{
  namespace transport
  {
    /// \brief Length of a GUID.
    #define GUID_STR_LEN (sizeof(uuid_t) * 2) + 4 + 1

    //////////////////////////////////////////////////
    std::string GetGuidStr(const uuid_t &_uuid)
    {
      std::vector<char> guid_str(GUID_STR_LEN);

      for (size_t i = 0; i < sizeof(uuid_t) && i != GUID_STR_LEN; ++i)
      {
        snprintf(&guid_str[0], GUID_STR_LEN,
          "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"
          , _uuid[0], _uuid[1], _uuid[2], _uuid[3],
          _uuid[4], _uuid[5], _uuid[6], _uuid[7],
          _uuid[8], _uuid[9], _uuid[10], _uuid[11],
          _uuid[12], _uuid[13], _uuid[14], _uuid[15]);
      }
      return std::string(guid_str.begin(), guid_str.end() - 1);
    }

/** \def IGNITION_VISIBLE
 * Use to represent "symbol visible" if supported
 */

/** \def IGNITION_HIDDEN
 * Use to represent "symbol hidden" if supported
 */

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define IGNITION_VISIBLE __attribute__ ((dllexport))
    #else
      #define IGNITION_VISIBLE __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define IGNITION_VISIBLE __attribute__ ((dllimport))
    #else
      #define IGNITION_VISIBLE __declspec(dllimport)
    #endif
  #endif
  #define IGNITION_HIDDEN
#else
  #if __GNUC__ >= 4
    #define IGNITION_VISIBLE __attribute__ ((visibility ("default")))
    #define IGNITION_HIDDEN  __attribute__ ((visibility ("hidden")))
  #else
    #define IGNITION_VISIBLE
    #define IGNITION_HIDDEN
  #endif
#endif
  }
}

#endif
