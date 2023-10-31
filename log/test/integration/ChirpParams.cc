/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#include "ChirpParams.hh"


static constexpr const char* kTopicChirpExe = TOPIC_CHIRP_EXE;

namespace gz::transport::log::test
{
  //////////////////////////////////////////////////
  /// \brief Similar to testing::forkAndRun(), except this function
  /// specifically calls the INTEGRATION_topicChirp_aux process and passes
  /// it arguments to determine how it should chirp out messages over its
  /// topics.
  /// \param _topics A list of topic names to chirp on
  /// \param _chirps The number of messages to chirp out. Each message
  /// will count up starting from the value 1 and ending with the value
  /// _chirps.
  /// \return A handle to the process. This can be used with
  /// testing::waitAndCleanupFork().
  gz::utils::Subprocess BeginChirps(
      const std::vector<std::string> &_topics,
      const int _chirps,
      const std::string &_partitionName)
  {
    // Argument list:
    // [0]: Executable name
    // [1]: Partition name
    // [2]: Number of chirps
    // [3]-[N]: Each topic name
    // [N+1]: Null terminator, required by execv
    const std::size_t numArgs = 3 + _topics.size() + 1;

    std::vector<std::string> strArgs;
    strArgs.reserve(numArgs-1);
    strArgs.push_back(kTopicChirpExe);
    strArgs.push_back(_partitionName);
    strArgs.push_back(std::to_string(_chirps));
    strArgs.insert(strArgs.end(), _topics.begin(), _topics.end());
    return gz::utils::Subprocess(strArgs);
  }
}  // namespace gz::transport::log::test
