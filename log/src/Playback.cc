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

#include <atomic>
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
#include "raii-sqlite3.hh"
#include "build_config.hh"


using namespace ignition::transport;
using namespace ignition::transport::log;

/// \brief Private implementation
class ignition::transport::log::Playback::Implementation
{
  /// \brief Default constructor
  public: Implementation();

  /// \brief Create a publisher of a given topic name and type
  /// \param[in] _topic Topic name to publish to
  /// \param[in] _type The message type name to publish
  /// \return True if a new publisher has been created. False if a publisher
  /// already existed for this combination.
  public: bool CreatePublisher(
              const std::string &_topic, const std::string &_type);

  /// \brief Begin playing messages in another thread
  /// \param[in] _batch The messages to be played back
  public: void StartPlayback(Batch _batch);

  /// \brief Wait until playback has finished playing
  public: void WaitUntilFinished();

  /// \brief True if the thread should be stopped
  public: std::atomic_bool stop;

  /// \brief True if a call to either AddTopic() has occurred.
  public: bool addTopicWasUsed;

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

  /// \brief Condition variable to wake up threads waiting on playback
  public: std::condition_variable waitConditionVariable;

  /// \brief Condition variable to wake up the playback thread if it's waiting
  /// to publish the next message.
  public: std::condition_variable stopConditionVariable;
};

//////////////////////////////////////////////////
Playback::Implementation::Implementation()
  : stop(true),
    addTopicWasUsed(false)
{
  // Do nothing
}

//////////////////////////////////////////////////
bool Playback::Implementation::CreatePublisher(
    const std::string &_topic, const std::string &_type)
{
  auto firstMapIter = this->publishers.find(_topic);
  if (firstMapIter == this->publishers.end())
  {
    // Create a map for the message topic
    this->publishers[_topic] = std::unordered_map<std::string,
      ignition::transport::Node::Publisher>();
    firstMapIter = this->publishers.find(_topic);
  }

  auto secondMapIter = firstMapIter->second.find(_type);
  if (secondMapIter != firstMapIter->second.end())
  {
    // A publisher which matches this (topic, type) pair has already been
    // created, so we stop here. We return a value of false because no new
    // publisher was created.
    return false;
  }

  // Create a publisher for the topic and type combo
  firstMapIter->second[_type] = this->node.Advertise(_topic, _type);
  LDBG("Creating publisher for " << _topic << " " << _type << "\n");
  return true;
}

//////////////////////////////////////////////////
void Playback::Implementation::StartPlayback(Batch _batch)
{
  this->stop = false;

  // Dev Note: batch is being initialized via the move-constructor within the
  // capture statement, as if it were being declared and constructed as
  // `auto batch{std::move(_batch)}`.
  //
  // For now, we need to use the move constructor because Batch does not yet
  // support copy construction. Also, the object named `_batch` will go out of
  // scope while this lambda is still in use, so we cannot rely on capturing it
  // by reference.
  this->playbackThread = std::thread(
    [this, batch{std::move(_batch)}] () mutable
    {
      bool publishedFirstMessage = false;

      // Get current elapsed on monotonic clock
      const std::chrono::nanoseconds startTime(
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
          const std::chrono::nanoseconds target =
              msg.TimeReceived() - firstMsgTime;

          // This will get initialized at if(!FinishedWaiting()) because the
          // FinishedWaiting lambda is capturing this by reference.
          std::chrono::nanoseconds now;

          // We create a lambda to test whether this thread needs to keep
          // waiting. This is used as a predicate by the
          // condition_variable::wait_for(~) function to avoid spurious wakeups.
          auto FinishedWaiting = [this, &startTime, &target, &now]() -> bool
          {
            now =
                std::chrono::steady_clock::now().time_since_epoch() - startTime;

            return target <= now || this->stop;
          };

          if (!FinishedWaiting())
          {
            // Passing a lock to wait_for is just a formality (we don't actually
            // want to unlock any mutex while waiting), so we create a temporary
            // mutex and lock to satisfy the function.
            //
            // According to the C++11 standard, it is undefined behavior to pass
            // different mutexes into the condition_variable::wait_for()
            // function of the same condition_variable instance from different
            // threads. However, this current thread should be the only thread
            // that is ever waiting on stopConditionVariable because we are
            // keeping playbackLock locked this whole time. Therefore, it's okay
            // for it lock its own local, unique mutex.
            //
            // This is really a substitute for the sleep_for function. This
            // alternative allows us to interrupt the sleep in case the user
            // calls Playback::Stop() while we are waiting between messages.
            std::mutex tempMutex;
            std::unique_lock<std::mutex> tempLock(tempMutex);
            this->stopConditionVariable.wait_for(
                  tempLock, target - now, FinishedWaiting);
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
void Playback::Implementation::WaitUntilFinished()
{
  if (this->logFile.Valid() && !this->stop)
  {
    std::unique_lock<std::mutex> lk(this->waitMutex);
    this->waitConditionVariable.wait(lk, [this]{return this->stop.load();});
  }
}

//////////////////////////////////////////////////
Playback::Playback(const std::string &_file)
  : dataPtr(new Implementation)
{
  if (!this->dataPtr->logFile.Open(_file, std::ios_base::in))
  {
    LERR("Could not open file [" << _file << "]\n");
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
PlaybackError Playback::Start(
    const std::chrono::nanoseconds &_waitAfterAdvertising)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  if (!this->dataPtr->logFile.Valid())
  {
    LERR("Failed to open log file\n");
    return PlaybackError::FAILED_TO_OPEN;
  }

  if (!this->dataPtr->addTopicWasUsed)
  {
    LDBG("No topics added, defaulting to all topics\n");
    int64_t numTopics = this->AddTopic(std::regex(".*"));
    if (numTopics < 0)
    {
      return static_cast<PlaybackError>(numTopics);
    }

    std::this_thread::sleep_for(_waitAfterAdvertising);
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
  return PlaybackError::SUCCESS;
}

//////////////////////////////////////////////////
PlaybackError Playback::Stop()
{
  if (!this->dataPtr->logFile.Valid())
  {
    return PlaybackError::FAILED_TO_OPEN;
  }

  this->dataPtr->stop = true;
  this->dataPtr->stopConditionVariable.notify_all();

  if (this->dataPtr->playbackThread.joinable())
    this->dataPtr->playbackThread.join();
  return PlaybackError::SUCCESS;
}

//////////////////////////////////////////////////
bool Playback::Valid() const
{
  return this->dataPtr->logFile.Valid();
}

//////////////////////////////////////////////////
void Playback::WaitUntilFinished()
{
  this->dataPtr->WaitUntilFinished();
}

//////////////////////////////////////////////////
PlaybackError Playback::AddTopic(const std::string &_topic)
{
  // We set this to true whether or not the function call succeeds, because by
  // calling this function, the user has expressed an intention to explicitly
  // specify which topics to publish.
  this->dataPtr->addTopicWasUsed = true;

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

  return PlaybackError::SUCCESS;
}

//////////////////////////////////////////////////
int64_t Playback::AddTopic(const std::regex &_topic)
{
  // We set this to true whether or not the function call succeeds, because by
  // calling this function, the user has expressed an intention to explicitly
  // specify which topics to publish.
  this->dataPtr->addTopicWasUsed = true;

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
