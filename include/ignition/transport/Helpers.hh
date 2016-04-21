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
#include <cstring>
#include <string>

/// \def IGNITION_VISIBLE
/// Use to represent "symbol visible" if supported

/// \def IGNITION_HIDDEN
/// Use to represent "symbol hidden" if supported

#if defined BUILDING_STATIC_LIBS
  #define IGNITION_VISIBLE
  #define IGNITION_MSGS_VISIBLE
  #define IGNITION_HIDDEN
#else
  #if defined __WIN32 || defined __CYGWIN__
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

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_IGNITION_MSGS
      #ifdef __GNUC__
       #define IGNITION_MSGS_VISIBLE __attribute__ ((dllexport))
      #else
       #define IGNITION_MSGS_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define IGNITION_MSGS_VISIBLE __attribute__ ((dllimport))
      #else
        #define IGNITION_MSGS_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define IGNITION_MSGS_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define IGNITION_MSGS_VISIBLE __attribute__ ((visibility ("default")))
      #define IGNITION_MSGS_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define IGNITION_MSGS_VISIBLE
      #define IGNITION_MSGS_HIDDEN
    #endif
  #endif
// BUILDING_STATIC_LIBS
#endif

 namespace ignition
 {
   namespace transport
   {
     /// \brief Find the environment variable '_name' and return its value.
     /// \param[in] _name Name of the environment variable.
     /// \param[out] _value Value if the variable was found.
     /// \return True if the variable was found or false otherwise.
     IGNITION_VISIBLE
     bool env(const std::string &_name,
              std::string &_value);
   }
 }

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

// __IGN_TRANSPORT_HELPERS_HH_INCLUDED__
#endif
