/*
 * Copyright (C) 2022 Open Source Robotics Foundation
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

#include <gz/transport/Helpers.hh>
#include <ignition/transport/config.hh>

// Avoid using deprecated message send/receive function when possible.
#if ZMQ_VERSION > ZMQ_MAKE_VERSION(4, 3, 1)
  #define IGN_ZMQ_POST_4_3_1 GZ_ZMQ_POST_4_3_1
#endif

// Avoid using deprecated set function when possible
#if CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 7, 0)
  // Ubuntu Focal (20.04) packages a different "4.7.0"
  #ifndef UBUNTU_FOCAL
    #define IGN_CPPZMQ_POST_4_7_0 GZ_CPPZMQ_POST_4_7_0
  #endif
#endif

#define ign_strcat gz_strcat
#define ign_strcpy gz_strcpy
#define ign_sprintf gz_sprintf
#define ign_strdup gz_strdup
