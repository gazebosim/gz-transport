/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include <ignition/common/Console.hh>
#include <ignition/transport/log/Log.hh>

#include <ignition/transport/log/Export.hh>

extern "C"
{
  /// \brief Sets verbosity of library
  /// \param[in] _level [0-4] Verbosity level
  void IGNITION_TRANSPORT_LOG_VISIBLE verbosity(int _level)
  {
    if (_level < 0 || _level > 4)
    {
      std::cerr << "Invalid verbosity level\n";
      std::exit(-1);
    }
    ignition::common::Console::SetVerbosity(_level);
  }

  /// \brief Record topics whose name matches the given pattern
  int IGNITION_TRANSPORT_LOG_VISIBLE recordTopics(
      const char *_file, const char *_pattern)
  {
    // TODO
    return 0;
  }

  /// \brief Playback topics whos name matches the given pattern
  int IGNITION_TRANSPORT_LOG_VISIBLE playbackTopics(
      const char *_file, const char *_pattern)
  {
    // TODO
    return 0;
  }
}
