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

#ifndef __IGN_TRANSPORT_IGN_HH_INCLUDED__
#define __IGN_TRANSPORT_IGN_HH_INCLUDED__

#include <cstring>

#include "ignition/transport/Helpers.hh"

/// \brief External hook to execute 'ign topic -i' from the command line.
/// \param[in] _topic Topic name.
extern "C" IGNITION_VISIBLE void cmdTopicInfo(const char *_topic);

/// \brief External hook to execute 'ign service -i' from the command line.
/// \param[in] _service Service name.
extern "C" IGNITION_VISIBLE void cmdServiceInfo(const char *_service);

/// \brief External hook to execute 'ign topic -l' from the command line.
extern "C" IGNITION_VISIBLE void cmdTopicList();

/// \brief External hook to execute 'ign service -l' from the command line.
extern "C" IGNITION_VISIBLE void cmdServiceList();

/// \brief External hook to read the library version.
/// \return C-string representing the version. Ex.: 0.1.2
extern "C" IGNITION_VISIBLE char *ignitionVersion();

#endif
