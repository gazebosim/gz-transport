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

#include <iostream>
#include <regex>

#include <ignition/transport/log/Export.hh>
#include <ignition/transport/log/Playback.hh>
#include <ignition/transport/log/Recorder.hh>
#include <ignition/transport/Node.hh>
#include "../Console.hh"
#include "LogCommandAPI.hh"

using namespace ignition;

//////////////////////////////////////////////////
int verbosity(int _level)
{
  if (_level < 0 || _level > 4)
  {
    std::cerr << "Invalid verbosity level\n";
    return INVALID_VERSION;
  }
  ignition::transport::log::__verbosity = _level;
  return SUCCESS;
}

//////////////////////////////////////////////////
int recordTopics(const char *_file, const char *_pattern)
{
  std::regex regexPattern;
  try
  {
    regexPattern = _pattern;
  }
  catch (const std::regex_error &e)
  {
    LERR("Regex pattern is invalid\n");
    return BAD_REGEX;
  }

  transport::log::Recorder recorder;

  if (recorder.AddTopic(regexPattern) < 0)
    return FAILED_TO_SUBSCRIBE;

  if (recorder.Start(_file) != transport::log::RecorderError::SUCCESS)
    return FAILED_TO_OPEN;

  // Wait until signaled (SIGINT, SIGTERM)
  transport::waitForShutdown();
  LDBG("Shutting down\n");
  recorder.Stop();

  return SUCCESS;
}

//////////////////////////////////////////////////
int playbackTopics(const char *_file, const char *_pattern, const int _wait_ms)
{
  std::regex regexPattern;
  try
  {
    regexPattern = _pattern;
  }
  catch (const std::regex_error &e)
  {
    LERR("Regex pattern is invalid\n");
    return BAD_REGEX;
  }

  transport::log::Playback player(_file);
  if (!player.Valid())
    return FAILED_TO_OPEN;

  if (player.AddTopic(regexPattern) < 0)
    return FAILED_TO_ADVERTISE;

  std::this_thread::sleep_for(std::chrono::milliseconds(_wait_ms));

  ignition::transport::log::PlaybackHandlePtr handler = player.Start();
  if (!handler)
    return FAILED_TO_OPEN;

  // Wait until playback finishes
  handler->WaitUntilFinished();
  LDBG("Shutting down\n");
  return SUCCESS;
}
