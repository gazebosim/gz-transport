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

#ifndef IGN_TRANSPORT_IGN_HH_
#define IGN_TRANSPORT_IGN_HH_

#include <cstring>

#include "ignition/transport/Export.hh"

/// \brief External hook to execute 'ign topic -i' from the command line.
/// \param[in] _topic Topic name.
extern "C" void cmdTopicInfo(const char *_topic);

/// \brief External hook to execute 'ign service -i' from the command line.
/// \param[in] _service Service name.
extern "C" void cmdServiceInfo(const char *_service);

/// \brief External hook to execute 'ign topic -l' from the command line.
extern "C" void cmdTopicList();

/// \brief External hook to execute 'ign service -l' from the command line.
extern "C" void cmdServiceList();

/// \brief External hook to execute 'ign topic -p' from the command line.
/// \param[in] _topic Topic name.
/// \param[in] _msgType Message type.
/// \param[in] _msgData The format expected is the same used by Protobuf
/// DebugString().
/// E.g.: cmdTopicPub("/foo", "ignition.msgs.StringMsg",
///                   "'data:\"Custom data\"');
extern "C" void cmdTopicPub(const char *_topic,
                                                       const char *_msgType,
                                                       const char *_msgData);

/// \brief External hook to execute 'ign service -r' from the command line.
/// \param[in] _service Service name.
/// \param[in] _reqType Message type used in the request.
/// \param[in] _repType Message type used in the response.
/// \param[in] _timeout The request will timeout after '_timeout' ms.
/// \param[in] _reqData Input data sent in the request.
/// The format expected is the same used by Protobuf DebugString().
/// E.g.: cmdServiceReq("/bar", "ignition.msgs.StringMsg",
///                     "ignition.msgs.StringMsg", 1000,
///                     "'data:\"Custom data\"');
extern "C" void cmdServiceReq(const char *_service,
                                                         const char *_reqType,
                                                         const char *_repType,
                                                         const int _timeout,
                                                         const char *_reqData);

extern "C" {
  /// \brief Enum used for specifing the message output format for functions
  /// like cmdTopicEcho.
  enum class MsgOutputFormat {
    // Default. Currently, this is Protobuf's DebugString output format.
    kDefault,

    // Output format used in Protobuf's Message::DebugString.
    kDebugString,

    // JSON output.
    kJSON
  };
}

/// \brief External hook to execute 'ign topic -e' from the command line.
/// The _duration parameter overrides the _count parameter.
/// \param[in] _topic Topic name.
/// \param[in] _duration Duration (seconds) to run. A value <= 0 indicates
/// no time limit. The _duration parameter overrides the _count parameter.
/// \param[in] _count Number of messages to echo and then stop. A value <= 0
/// indicates no limit. The _duration parameter overrides the _count
/// parameter.
/// \param[in] _outputFormat Message output format.
extern "C" void cmdTopicEcho(const char *_topic, const double _duration,
                             int _count, MsgOutputFormat _outputFormat);

/// \brief External hook to read the library version.
/// \return C-string representing the version. Ex.: 0.1.2
extern "C" const char *ignitionVersion();

#endif
