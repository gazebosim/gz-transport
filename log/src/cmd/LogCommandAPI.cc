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
#include <ignition/transport/log/Log.hh>
#include <ignition/transport/log/Export.hh>

using namespace ignition;

extern "C"
{
  enum {
    NO_ERROR = 0,
    FAILED_TO_OPEN = 1,
    FAILED_TO_SUBSCRIBE = 2,
    BAD_REGEX = 3,
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
    transport::log::Log logFile;
    if (!logFile.Open(_file, transport::log::READ_WRITE_CREATE))
    {
      ignerr << "Failed to open or create file [" << _file << "]\n";
      return FAILED_TO_OPEN;
    }

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

    // Set up to use monotonic time for accurate delay between messages
    std::chrono::nanoseconds wallStartNS(std::chrono::seconds(std::time(NULL)));
    std::chrono::nanoseconds monoStartNS(
        std::chrono::steady_clock::now().time_since_epoch());

    // Get a value to compare wall clock with monotonic clock
    std::chrono::nanoseconds wallMinusMonoNS = wallStartNS - monoStartNS;

    // Create a callback capturing some local variables
    std::function<void(
        const google::protobuf::Message &, const transport::MessageInfo &)>
      cb = [&logFile, &wallMinusMonoNS](
          const google::protobuf::Message &_msg,
          const transport::MessageInfo &_info)
      {
        // Get time RX using monotonic 
        std::chrono::nanoseconds nowNS(
            std::chrono::steady_clock::now().time_since_epoch());
        // monotonic -> utc in nanoseconds
        std::chrono::nanoseconds utcNS = wallMinusMonoNS + nowNS;
        // Round to nearest second
        std::chrono::seconds utcS =
          std::chrono::duration_cast<std::chrono::seconds>(utcNS);
        // create time object with rounded seconds and remainder nanoseconds
        common::Time timeRX(utcS.count(),
            (utcNS - std::chrono::nanoseconds(utcS)).count());

        // TODO use raw bytes subscriber
        std::string buffer;
        _msg.SerializeToString(&(buffer));

        igndbg << "Received message\n";
        if (!logFile.InsertMessage(
              timeRX,
              _info.Topic(),
              _info.Type(),
              reinterpret_cast<const void *>(buffer.c_str()),
              buffer.size()))
        {
          ignwarn << "Failed to insert message into log file\n";
        }
      };

    transport::Node node;

    // TODO what if topics do not exist yet? How does record discover them?
    std::vector<std::string> all_topics;
    node.TopicList(all_topics);
    for (auto topic : all_topics)
    {
      // TODO if topic matches pattern
      if (std::regex_match(topic, regexPattern))
      {
        ignmsg << "Recording " << topic << "\n";
        // Subscribe to the topic
        if (!node.Subscribe(topic, cb))
        {
          ignerr << "Failed to subscribe to [" << topic << "]\n";
          return FAILED_TO_SUBSCRIBE;
        }
      }
      else
      {
        igndbg << "Not recording " << topic << "\n";
      }
    }

    // Wait until signaled (SIGINT, SIGTERM)
    transport::waitForShutdown();
    igndbg << "Shutting down\n";
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
