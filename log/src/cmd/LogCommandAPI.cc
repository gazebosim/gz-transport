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
#include <functional>
#include <regex>

#include <ignition/common/Console.hh>
#include <ignition/transport/Node.hh>
#include <ignition/transport/log/Playback.hh>
#include <ignition/transport/log/Record.hh>
#include <ignition/transport/log/Export.hh>

using namespace ignition;

extern "C"
{
  enum {
    NO_ERROR = 0,
    FAILED_TO_OPEN = 1,
    BAD_REGEX = 2,
  };

  /// \brief Sets verbosity of library
  /// \param[in] _level [0-4] Verbosity level
  void IGNITION_TRANSPORT_LOG_VISIBLE verbosity(int _level)
  {
    if (_level < 0 || _level > 4)
    {
      std::cerr << "Invalid verbosity level\n";
      std::exit(-1);
    }
    common::Console::SetVerbosity(_level);
  }

  /// \brief Record topics whose name matches the given pattern
  int IGNITION_TRANSPORT_LOG_VISIBLE recordTopics(
      const char *_file, const char *_pattern)
  {
    std::regex regexPattern;
    try
    {
      regexPattern = _pattern;
    }
    catch (std::regex_error e)
    {
      ignerr << "Regex pattern is invalid\n";
      return BAD_REGEX;
    }

    transport::log::Record recorder;
    recorder.AddTopic(regexPattern);
    if (recorder.Start(_file) != transport::log::RecordError::NO_ERROR)
    {
      return FAILED_TO_OPEN;
    }

    // Wait until signaled (SIGINT, SIGTERM)
    transport::waitForShutdown();
    igndbg << "Shutting down\n";
    recorder.Stop();

    return NO_ERROR;
  }

  /// \brief Playback topics whos name matches the given pattern
  /// \param[in] _file Path to the log file to playback
  /// \param[in] _pattern ECMAScript regular expression to match against topics
  int IGNITION_TRANSPORT_LOG_VISIBLE playbackTopics(
      const char *_file, const char *_pattern)
  {
    std::regex regexPattern;
    try
    {
      regexPattern = _pattern;
    }
    catch (std::regex_error e)
    {
      ignerr << "Regex pattern is invalid\n";
      return BAD_REGEX;
    }

    transport::log::Playback player(_file);
    player.AddTopic(regexPattern);
    if (player.Start() != transport::log::PlaybackError::NO_ERROR)
    {
      return FAILED_TO_OPEN;
    }

    // Wait until signaled (SIGINT, SIGTERM)
    transport::waitForShutdown();
    igndbg << "Shutting down\n";
    player.Stop();
    return NO_ERROR;
  }
}
