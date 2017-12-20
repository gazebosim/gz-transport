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

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

#include <ignition/common/Console.hh>
#include <ignition/transport/Node.hh>

#include "ignition/transport/log/Record.hh"
#include "ignition/transport/log/Log.hh"
#include "src/raii-sqlite3.hh"
#include "build_config.hh"


using namespace ignition::transport;
using namespace ignition::transport::log;

/// \brief Private implementation
class ignition::transport::log::RecordPrivate
{
  /// \brief Subscriber callback
  public: void OnMessageReceived(
          const google::protobuf::Message &_msg,
          const transport::MessageInfo &_info);

  /// \brief log file or nullptr if not recording
  public: std::unique_ptr<Log> logFile;

  /// \brief Value added to steady_clock giving time in UTC
  public: std::chrono::nanoseconds wallMinusMono;

  /// \brief mutex for thread safety with log file
  public: std::mutex logFileMutex;

  /// \brief node used to create subscriptions
  public: ignition::transport::Node node;
};

//////////////////////////////////////////////////
void RecordPrivate::OnMessageReceived(
          const google::protobuf::Message &_msg,
          const transport::MessageInfo &_info)
{
  // Get time RX using monotonic
  std::chrono::nanoseconds nowNS(
      std::chrono::steady_clock::now().time_since_epoch());
  // monotonic -> utc in nanoseconds
  std::chrono::nanoseconds utcNS = this->wallMinusMono + nowNS;
  // Round to nearest second
  std::chrono::seconds utcS =
    std::chrono::duration_cast<std::chrono::seconds>(utcNS);
  // create time object with rounded seconds and remainder nanoseconds
  common::Time timeRX(utcS.count(),
      (utcNS - std::chrono::nanoseconds(utcS)).count());

  // TODO use raw bytes subscriber
  std::string buffer;
  _msg.SerializeToString(&(buffer));

  igndbg << "RX'" << _info.Topic() << "'[" << _info.Type() << "]\n";

  std::lock_guard<std::mutex> lock(this->logFileMutex);
  if (this->logFile && !this->logFile->InsertMessage(
        timeRX,
        _info.Topic(),
        _info.Type(),
        reinterpret_cast<const void *>(buffer.c_str()),
        buffer.size()))
  {
    ignwarn << "Failed to insert message into log file\n";
  }
}

//////////////////////////////////////////////////
Record::Record()
  : dataPtr(new RecordPrivate)
{
  // Set the offset used to get UTC from steady clock
  std::chrono::nanoseconds wallStartNS(std::chrono::seconds(std::time(NULL)));
  std::chrono::nanoseconds monoStartNS(
      std::chrono::steady_clock::now().time_since_epoch());
  this->dataPtr->wallMinusMono = wallStartNS - monoStartNS;
}

//////////////////////////////////////////////////
Record::Record(Record &&_other)  // NOLINT
  : dataPtr(std::move(_other.dataPtr))
{
}

//////////////////////////////////////////////////
Record::~Record()
{
  this->Stop();
}

//////////////////////////////////////////////////
RecordError Record::Start(const std::string &_file)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  if (this->dataPtr->logFile)
  {
    ignwarn << "Recording is already in progress\n";
    return RecordError::ALREADY_RECORDING;
  }

  this->dataPtr->logFile.reset(new Log());
  if (!this->dataPtr->logFile->Open(_file, std::ios_base::out))
  {
    ignerr << "Failed to open or create file [" << _file << "]\n";
    this->dataPtr->logFile.reset(nullptr);
    return RecordError::FAILED_TO_OPEN;
  }

  ignmsg << "Started recording to [" << _file << "]\n";

  return RecordError::NO_ERROR;
}

//////////////////////////////////////////////////
void Record::Stop()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  this->dataPtr->logFile.reset(nullptr);
}

//////////////////////////////////////////////////
RecordError Record::AddTopic(const std::string &_topic)
{
  igndbg << "Recording [" << _topic << "]\n";
  // Subscribe to the topic whether it exists or not
  if (!this->dataPtr->node.Subscribe(
        _topic, &RecordPrivate::OnMessageReceived, this->dataPtr.get()))
  {
    ignerr << "Failed to subscribe to [" << _topic << "]\n";
    return RecordError::FAILED_TO_SUBSCRIBE;
  }
  return RecordError::NO_ERROR;
}

//////////////////////////////////////////////////
int Record::AddTopic(const std::regex &_topic)
{
  int numSubscriptions = 0;
  std::vector<std::string> allTopics;
  this->dataPtr->node.TopicList(allTopics);
  for (auto topic : allTopics)
  {
    if (std::regex_match(topic, _topic))
    {
      igndbg << "Recording " << topic << "\n";
      // Subscribe to the topic
      if (!this->dataPtr->node.Subscribe(
            topic, &RecordPrivate::OnMessageReceived, this->dataPtr.get()))
      {
        ignerr << "Failed to subscribe to [" << topic << "]\n";
        return static_cast<int>(RecordError::FAILED_TO_SUBSCRIBE);
      }
      ++numSubscriptions;
    }
    else
    {
      igndbg << "Not recording " << topic << "\n";
    }
  }
  // TODO store pattern and attempt to subscribe to it when discovered
  return numSubscriptions;
}
