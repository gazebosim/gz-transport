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
#include <ignition/transport/Discovery.hh>

#include "ignition/transport/log/Recorder.hh"
#include "ignition/transport/log/Log.hh"
#include "src/raii-sqlite3.hh"
#include "build_config.hh"


using namespace ignition::transport;
using namespace ignition::transport::log;

/// \brief Private implementation
class ignition::transport::log::RecorderPrivate
{
  /// \brief Subscriber callback
  public: void OnMessageReceived(
          const char *_data,
          std::size_t _len,
          const transport::MessageInfo &_info);

  /// \brief Callback that listens for newly advertised topics
  public: void OnAdvertisement(const Publisher &_publisher);

  /// \sa Record::AddTopic(const std::string&)
  public: RecorderError AddTopic(const std::string &_topic);

  /// \sa Record::AddTopic(const std::regex&)
  public: int AddTopic(const std::regex &_pattern);

  /// \brief log file or nullptr if not recording
  public: std::unique_ptr<Log> logFile;

  /// \brief A set of topic patterns that we want to subscribe to
  public: std::vector<std::regex> patterns;

  /// \brief A set of topic names that we have already subscribed to. When new
  /// publishers advertise topics that we are already subscribed to, our
  /// OnAdvertisement callback can just ignore it.
  public: std::set<std::string> alreadySubscribed;

  /// \brief mutex for thread safety when evaluating newly advertised topics
  public: std::mutex topicMutex;

  /// \brief Value added to steady_clock giving time in UTC
  public: std::chrono::nanoseconds wallMinusMono;

  /// \brief mutex for thread safety with log file
  public: std::mutex logFileMutex;

  /// \brief node used to create subscriptions
  public: Node node;

  /// \brief callback used on every subscriber
  public: RawCallback rawCallback;

  /// \brief Object for discovering new publishers as they advertise themselves
  public: std::unique_ptr<MsgDiscovery> discovery;
};

//////////////////////////////////////////////////
void RecorderPrivate::OnMessageReceived(
          const char *_data,
          std::size_t _len,
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

  igndbg << "RX'" << _info.Topic() << "'[" << _info.Type() << "]\n";

  std::lock_guard<std::mutex> lock(this->logFileMutex);

  // Note: this->logFile will only be a nullptr before Start() has been called
  // or after Stop() has been called. If it is a nullptr, then we are not
  // recording anything yet, so we can just skip inserting the message.
  if (this->logFile && !this->logFile->InsertMessage(
        timeRX,
        _info.Topic(),
        _info.Type(),
        reinterpret_cast<const void *>(_data),
        _len))
  {
    ignwarn << "Failed to insert message into log file\n";
  }
}

//////////////////////////////////////////////////
void RecorderPrivate::OnAdvertisement(const Publisher &_publisher)
{
  std::string partition;
  std::string topic;

  TopicUtils::DecomposeFullyQualifiedTopic(
        _publisher.Topic(), partition, topic);

  const std::string &nodePartition = this->node.Options().Partition();

  // If the first character in our node's partition is a slash, then we should
  // start our comparison from index 0 of the incoming publisher's partition.
  // Otherwise, we should start the comparison from index 1. The incoming
  // publisher's partition is guaranteed to begin with the forward slash.
  const std::size_t startCmp = nodePartition[0] == '/' ? 0 : 1;

  // If the advertised partition does not match ours, ignore this advertisement
  if (strcmp(nodePartition.c_str(), partition.c_str() + startCmp) != 0)
    return;

  // If we are already subscribed to the topic, ignore this advertisement
  if (this->alreadySubscribed.find(topic) != this->alreadySubscribed.end())
    return;

  for (const std::regex &pattern : this->patterns)
  {
    if (std::regex_match(topic, pattern))
    {
      this->AddTopic(topic);
    }
  }
}

//////////////////////////////////////////////////
RecorderError RecorderPrivate::AddTopic(const std::string &_topic)
{
  igndbg << "Recording [" << _topic << "]\n";
  // Subscribe to the topic whether it exists or not
  if (!this->node.SubscribeRaw(_topic, this->rawCallback))
  {
    ignerr << "Failed to subscribe to [" << _topic << "]\n";
    return RecorderError::FAILED_TO_SUBSCRIBE;
  }

  this->alreadySubscribed.insert(_topic);

  return RecorderError::NO_ERROR;
}

//////////////////////////////////////////////////
int RecorderPrivate::AddTopic(const std::regex &_pattern)
{
  int numSubscriptions = 0;
  std::vector<std::string> allTopics;
  this->node.TopicList(allTopics);
  for (auto topic : allTopics)
  {
    if (std::regex_match(topic, _pattern))
    {
      // Subscribe to the topic
      if (this->AddTopic(topic) == RecorderError::FAILED_TO_SUBSCRIBE)
      {
        return static_cast<int>(RecorderError::FAILED_TO_SUBSCRIBE);
      }
      ++numSubscriptions;
    }
    else
    {
      igndbg << "Not recording " << topic << "\n";
    }
  }

  this->patterns.push_back(_pattern);

  return numSubscriptions;
}

//////////////////////////////////////////////////
Recorder::Recorder()
  : dataPtr(new RecorderPrivate)
{
  // Set the offset used to get UTC from steady clock
  std::chrono::nanoseconds wallStartNS(std::chrono::seconds(std::time(NULL)));
  std::chrono::nanoseconds monoStartNS(
      std::chrono::steady_clock::now().time_since_epoch());
  this->dataPtr->wallMinusMono = wallStartNS - monoStartNS;

  // Make a lambda to wrap a member function callback
  this->dataPtr->rawCallback = [this](
      const char *_data, std::size_t _len, const transport::MessageInfo &_info)
  {
    this->dataPtr->OnMessageReceived(_data, _len, _info);
  };

  Uuid uuid;
  this->dataPtr->discovery = std::unique_ptr<MsgDiscovery>(
        new MsgDiscovery(uuid.ToString(), NodeShared::kMsgDiscPort));

  DiscoveryCallback<Publisher> cb = [this](const Publisher &_publisher)
  {
    this->dataPtr->OnAdvertisement(_publisher);
  };

  this->dataPtr->discovery->ConnectionsCb(cb);
  this->dataPtr->discovery->Start();
}

//////////////////////////////////////////////////
Recorder::Recorder(Recorder &&_other)  // NOLINT
  : dataPtr(std::move(_other.dataPtr))
{
}

//////////////////////////////////////////////////
Recorder::~Recorder()
{
  this->Stop();
}

//////////////////////////////////////////////////
RecorderError Recorder::Start(const std::string &_file)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  if (this->dataPtr->logFile)
  {
    ignwarn << "Recording is already in progress\n";
    return RecorderError::ALREADY_RECORDING;
  }

  this->dataPtr->logFile.reset(new Log());
  if (!this->dataPtr->logFile->Open(_file, std::ios_base::out))
  {
    ignerr << "Failed to open or create file [" << _file << "]\n";
    this->dataPtr->logFile.reset(nullptr);
    return RecorderError::FAILED_TO_OPEN;
  }

  ignmsg << "Started recording to [" << _file << "]\n";

  return RecorderError::NO_ERROR;
}

//////////////////////////////////////////////////
void Recorder::Stop()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  this->dataPtr->logFile.reset(nullptr);
}

//////////////////////////////////////////////////
RecorderError Recorder::AddTopic(const std::string &_topic)
{
  return this->dataPtr->AddTopic(_topic);
}

//////////////////////////////////////////////////
int Recorder::AddTopic(const std::regex &_topic)
{
  return this->dataPtr->AddTopic(_topic);
}
