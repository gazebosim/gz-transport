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

#include <ignition/common/Console.hh>
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
  /// \brief True if the thread should stop
  public: bool stop = false;

  /// \brief thread running playback
  public: std::thread playbackThread;

  /// \brief log file or nullptr if not playing
  public: std::unique_ptr<Log> logFile;

  /// \brief mutex for thread safety with log file
  public: std::mutex logFileMutex;

  /// \brief node used to create publishers
  public: ignition::transport::Node node;
};


//////////////////////////////////////////////////
Playback::Playback()
  : dataPtr(new PlaybackPrivate)
{
}

//////////////////////////////////////////////////
Playback::Playback(Playback &&_other)  // NOLINT
  : dataPtr(std::move(_other.dataPtr))
{
}

//////////////////////////////////////////////////
Playback::~Playback()
{
  this->Stop();
}

//////////////////////////////////////////////////
PlaybackError Playback::Start(const std::string &_file)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->logFileMutex);
  if (this->dataPtr->logFile)
  {
    ignwarn << "Playing is already in progress\n";
    return PlaybackError::ALREADY_PLAYING;
  }

  this->dataPtr->logFile.reset(new Log());
  if (!this->dataPtr->logFile->Open(_file, std::ios_base::in))
  {
    ignerr << "Failed to open file [" << _file << "]\n";
    this->dataPtr->logFile.reset(nullptr);
    return PlaybackError::FAILED_TO_OPEN;
  }

  auto iter = this->dataPtr->logFile->AllMessages();
  if (MsgIter() == iter)
  {
    ignwarn << "There are no messages to play\n";
    return PlaybackError::NO_MESSAGES;
  }

  ignmsg << "Started playing from [" << _file << "]\n";

  // Dev Note: msgIter is being initialized via the move-constructor within the
  // capture statement, as if it were being declared and constructed as
  // `auto msgIter{std::move(iter)}`.
  //
  // For now, we need to use the move constructor because MsgIter does not yet
  // support copy construction. Also, the object named `iter` will go out of
  // scope while this lambda is still in use, so we cannot rely on capturing it
  // by reference.
  this->dataPtr->playbackThread = std::thread(
    [this, msgIter{std::move(iter)}] () mutable
    {
      bool publishedFirstMessage = false;
      // Map of topic names to a map of message types to publisher
      std::unordered_map<std::string,
        std::unordered_map<std::string,
        ignition::transport::Node::Publisher>> publishers;

      ignition::common::Time nextTime;
      ignition::common::Time lastMessageTime;

      std::lock_guard<std::mutex> playbackLock(this->dataPtr->logFileMutex);
      while (!this->dataPtr->stop && MsgIter() != msgIter)
      {
        // Create a publisher when a new topic/type is discovered
        // TODO create all publishers at the start
        auto firstMapIter = publishers.find(msgIter->Topic());
        ignition::transport::Node::Publisher *pub = nullptr;
        if (firstMapIter == publishers.end())
        {
          // Create a map for the message topic
          publishers[msgIter->Topic()] = std::unordered_map<std::string,
            ignition::transport::Node::Publisher>();
          firstMapIter = publishers.find(msgIter->Topic());
        }
        auto secondMapIter = firstMapIter->second.find(msgIter->Type());
        if (secondMapIter != firstMapIter->second.end())
        {
          pub = &(secondMapIter->second);
        }
        else
        {
          // Create a publisher for the topic and type combo
          firstMapIter->second[msgIter->Type()] = this->dataPtr->node.Advertise(
              msgIter->Topic(), msgIter->Type());
          igndbg << "Creating publisher for " <<
            msgIter->Topic() << " " << msgIter->Type() << "\n";
          pub = &(firstMapIter->second[msgIter->Type()]);
        }

        //Publish the first message right away, all others delay
        if (publishedFirstMessage)
        {
          // TODO use steady_clock to track the time spent publishing the last
          // message and alter the delay accordingly
          ignition::common::Time delta = nextTime - lastMessageTime;

          std::this_thread::sleep_for(std::chrono::nanoseconds(
                delta.sec * 1000000000 + delta.nsec));
        }
        else
        {
          publishedFirstMessage = true;
        }

        // Actually publish the message
        const Message &msg = *msgIter;
        igndbg << "publishing\n";
        pub->RawPublish(msg.Data(), msg.Type());
        lastMessageTime = nextTime;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ++msgIter;
      }
    });

  return PlaybackError::NO_ERROR;
}

//////////////////////////////////////////////////
void Playback::Stop()
{
  this->dataPtr->stop = true;
  this->dataPtr->playbackThread.join();
  this->dataPtr->logFile.reset(nullptr);
}
