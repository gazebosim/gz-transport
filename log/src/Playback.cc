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
#include <unordered_map>
#include <unordered_set>

#include "Console.hh"
#include <ignition/transport/Node.hh>

#include "ignition/transport/log/Playback.hh"
#include "ignition/transport/log/Log.hh"
#include "src/raii-sqlite3.hh"
#include "build_config.hh"


using namespace ignition::transport;
using namespace ignition::transport::log;

/// \brief Private implementation
class ignition::transport::log::PlaybackPrivate
{
  /// \brief Create a publisher of a given topic name and type
  public: bool CreatePublisher(
              const std::string &_topic, const std::string &_type);

  /// \brief Begin playing messages in another thread
  /// \param[in] _batch The messages to be played back
  public: void StartPlayback(Batch _batch);

  /// \brief Wait until playback has finished playing
  public: void WaitUntilFinished();

  /// \brief True if the thread should be stopped
  public: bool stop = true;

  /// \brief thread running playback
  public: std::thread playbackThread;

  /// \brief log file to play from
  public: Log logFile;

  /// \brief mutex for thread safety with log file
  public: std::mutex logFileMutex;

  /// \brief node used to create publishers
  public: ignition::transport::Node node;

  /// \brief topics that are being played back
  public: std::unordered_set<std::string> topicNames;

  /// \brief Map whose key is a topic name and value is another map whose
  /// key is a message type name and value is a publisher
  public: std::unordered_map<std::string,
          std::unordered_map<std::string,
            ignition::transport::Node::Publisher>> publishers;

  /// \brief a mutex to use when waiting for playback to finish
  public: std::mutex waitMutex;

  /// \brief Condition variable to wakeup threads waiting on playback
  public: std::condition_variable waitConditionVariable;
};

//////////////////////////////////////////////////
bool PlaybackPrivate::CreatePublisher(
    const std::string &_topic, const std::string &_type)
{
  auto firstMapIter = this->publishers.find(_topic);
  if (firstMapIter == this->publishers.end())
  {
    // Create a map for the message topic
    this->publishers[_topic] = std::unordered_map<std::string,
      ignition::transport::Node::Publisher>();
    firstMapIter = publishers.find(_topic);
  }

  auto secondMapIter = firstMapIter->second.find(_type);
  if (secondMapIter != firstMapIter->second.end())
  {
    return false;
  }

  // Create a publisher for the topic and type combo
  firstMapIter->second[_type] = this->node.Advertise(
      _topic, _type);
  LDBG("Creating publisher for " << _topic << " " << _type << "\n");
  return true;
}

//////////////////////////////////////////////////
void PlaybackPrivate::StartPlayback(Batch _batch)
{
  this->stop = false;

  // Dev Note: msgIter is being initialized via the move-constructor within the
  // capture statement, as if it were being declared and constructed as
  // `auto msgIter{std::move(iter)}`.
  //
  // For now, we need to use the move constructor because MsgIter does not yet
  // support copy construction. Also, the object named `iter` will go out of
  // scope while this lambda is still in use, so we cannot rely on capturing it
  // by reference.
  this->playbackThread = std::thread(
    [this, batch{std::move(_batch)}] () mutable
    {
      bool publishedFirstMessage = false;

      // Get current elapsed on monotonic clock
      std::chrono::nanoseconds startTime(
          std::chrono::steady_clock::now().time_since_epoch());
      std::chrono::nanoseconds firstMsgTime;

      std::lock_guard<std::mutex> playbackLock(this->logFileMutex);
      for (const Message &msg : batch)
      {
        if (this->stop)
        {
          break;
        }

        // Publish the first message right away, all others delay
        if (publishedFirstMessage)
        {
          std::chrono::nanoseconds target = msg.TimeReceived() - firstMsgTime;
          auto now = std::chrono::nanoseconds(
              std::chrono::steady_clock::now().time_since_epoch());
          now -= startTime;
          if (target > now)
          {
            std::this_thread::sleep_for(target - now);
          }
        }
        else
        {
          publishedFirstMessage = true;
          firstMsgTime = msg.TimeReceived();
        }

        // Actually publish the message
        LDBG("publishing\n");
        this->publishers[msg.Topic()][msg.Type()].PublishRaw(
            msg.Data(), msg.Type());
      }
      {
        std::lock_guard<std::mutex> lk(this->waitMutex);
        this->stop = true;
      }
      this->waitConditionVariable.notify_all();
    });
}

//////////////////////////////////////////////////
void PlaybackPrivate::WaitUntilFinished()
{
  if (this->logFile.Valid() && !this->stop)
  {
    std::unique_lock<std::mutex> lk(this->waitMutex);
    this->waitConditionVariable.wait(lk, [this]{return this->stop;});
  }
}

//////////////////////////////////////////////////
Playback::Playback(const std::string &_file)
  : dataPtr(new PlaybackPrivate)
{
  if (!this->dataPtr->logFile.Open(_file, std::ios_base::in))
  {
    LERR("Failed to open file [" << _file << "]\n");
  }
  else
  {
    LDBG("Playback opened file [" << _file << "]\n");
  }
}

//////////////////////////////////////////////////
Playback::Playback(Playback &&_other)  // NOLINT
  : dataPtr(std::move(_other.dataPtr))
{
}

//////////////////////////////////////////////////
Playback::~Playback()
{
  if (this->dataPtr)
  {
    this->Stop();
  }
}

//////////////////////////////////////////////////
PlaybackError Playback::Start()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  if (!this->dataPtr->logFile.Valid())
  {
    LERR("Failed to open log file\n");
    return PlaybackError::FAILED_TO_OPEN;
  }

  if (this->dataPtr->publishers.empty())
  {
    LDBG("No topics added, defaulting to all topics\n");
    int64_t numTopics = this->AddTopic(std::regex(".*"));
    if (numTopics < 0)
    {
      return static_cast<PlaybackError>(numTopics);
    }
  }

  Batch batch = this->dataPtr->logFile.QueryMessages(
        TopicList::Create(this->dataPtr->topicNames));
  if (batch.begin() == batch.end())
  {
    LWRN("There are no messages to play\n");
    return PlaybackError::NO_MESSAGES;
  }

  this->dataPtr->StartPlayback(std::move(batch));
  LMSG("Started playing\n");
  return PlaybackError::NO_ERROR;
}

//////////////////////////////////////////////////
PlaybackError Playback::Stop()
{
  if (!this->dataPtr->logFile.Valid())
  {
    LERR("Failed to open log file\n");
    return PlaybackError::FAILED_TO_OPEN;
  }
  this->dataPtr->stop = true;
  if (this->dataPtr->playbackThread.joinable())
    this->dataPtr->playbackThread.join();
  return PlaybackError::NO_ERROR;
}

//////////////////////////////////////////////////
void Playback::WaitUntilFinished()
{
  this->dataPtr->WaitUntilFinished();
}

//////////////////////////////////////////////////
PlaybackError Playback::AddTopic(const std::string &_topic)
{
  if (!this->dataPtr->logFile.Valid())
  {
    LERR("Failed to open log file\n");
    return PlaybackError::FAILED_TO_OPEN;
  }

  const Descriptor *desc = this->dataPtr->logFile.Descriptor();
  const Descriptor::NameToMap &allTopics = desc->TopicsToMsgTypesToId();

  const Descriptor::NameToMap::const_iterator it = allTopics.find(_topic);
  if (it == allTopics.end())
  {
    LWRN("Topic [" << _topic << "] is not in the log\n");
    return PlaybackError::NO_SUCH_TOPIC;
  }

  const std::string &topic = it->first;
  for (const auto &typeEntry : it->second)
  {
    const std::string &type = typeEntry.first;
    LDBG("Playing back [" << _topic << "]\n");
    // Save the topic name for the query in Start
    this->dataPtr->topicNames.insert(_topic);
    if (!this->dataPtr->CreatePublisher(topic, type))
    {
      return PlaybackError::FAILED_TO_ADVERTISE;
    }
  }

  return PlaybackError::NO_ERROR;
}

//////////////////////////////////////////////////
int64_t Playback::AddTopic(const std::regex &_topic)
{
  if (!this->dataPtr->logFile.Valid())
  {
    LERR("Failed to open log file\n");
    return static_cast<int>(PlaybackError::FAILED_TO_OPEN);
  }

  int64_t numPublishers = 0;
  const Descriptor *desc = this->dataPtr->logFile.Descriptor();
  const Descriptor::NameToMap &allTopics = desc->TopicsToMsgTypesToId();

  for (const auto &topicEntry : allTopics)
  {
    const std::string &topic = topicEntry.first;
    if (!std::regex_match(topic, _topic))
      continue;

    for (const auto &typeEntry : topicEntry.second)
    {
      const std::string &type = typeEntry.first;

      this->dataPtr->topicNames.insert(topic);
      if (this->dataPtr->CreatePublisher(topic, type))
      {
        ++numPublishers;
      }
      else
      {
        return static_cast<int>(PlaybackError::FAILED_TO_ADVERTISE);
      }
    }
  }
  return numPublishers;
}
