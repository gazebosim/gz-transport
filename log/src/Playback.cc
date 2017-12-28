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

  this->dataPtr->playbackThread = std::thread(
    [this, iter{std::move(iter)}] () mutable
    {
      bool publishedFirstMessage = false;
      // Map of topic names to a map of message types to publisher
      std::unordered_map<std::string,
        std::unordered_map<std::string,
        ignition::transport::Node::Publisher>> publishers;

      ignition::common::Time nextTime;
      ignition::common::Time lastMessageTime;

      std::lock_guard<std::mutex> playbackLock(this->dataPtr->logFileMutex);
      while (!this->dataPtr->stop && MsgIter() != iter)
      {
        // Create a publisher when a new topic/type is discovered
        // TODO create all publishers at the start
        auto firstMapIter = publishers.find(iter->Topic());
        ignition::transport::Node::Publisher *pub = nullptr;
        if (firstMapIter == publishers.end())
        {
          // Create a map for the message topic
          publishers[iter->Topic()] = std::unordered_map<std::string,
            ignition::transport::Node::Publisher>();
          firstMapIter = publishers.find(iter->Topic());
        }
        auto secondMapIter = firstMapIter->second.find(iter->Type());
        if (secondMapIter != firstMapIter->second.end())
        {
          pub = &(secondMapIter->second);
        }
        else
        {
          // Create a publisher for the topic and type combo
          firstMapIter->second[iter->Type()] = this->dataPtr->node.Advertise(
              iter->Topic(), iter->Type());
          igndbg << "Creating publisher for " <<
            iter->Topic() << " " << iter->Type() << "\n";
          pub = &(firstMapIter->second[iter->Type()]);
        }

        //Publish the first message right away, all others delay
        if (publishedFirstMessage)
        {
          // TODO use steady_clock to track the time spent publishing the last message
          // and alter the delay accordingly
          ignition::common::Time delta = nextTime - lastMessageTime;

          std::this_thread::sleep_for(std::chrono::nanoseconds(
                delta.sec * 1000000000 + delta.nsec));
        }
        else
        {
          publishedFirstMessage = true;
        }

        // Do some stuff that won't be needed with a raw publisher
        // of ignition messages this executable is linked with
        const google::protobuf::Descriptor* descriptor =
          google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(iter->Type());
        if (!descriptor)
        {
          ignerr << "This message cannot be published until ignition transport has a raw_bytes publisher(1)\n";
          continue;
        }
        const google::protobuf::Message* prototype = NULL;
        prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);

        if (!prototype)
        {
          ignerr << "This message cannot be published until ignition transport has a raw_bytes publisher(2)\n";
          continue;
        }

        // Actually publish the message
        google::protobuf::Message* payload = prototype->New();
        payload->ParseFromString(iter->Data());
        igndbg << "publishing\n";
        pub->Publish(*payload);
        lastMessageTime = nextTime;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ++iter;
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
