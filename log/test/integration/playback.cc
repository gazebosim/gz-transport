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

#include <gtest/gtest.h>

#include <ignition/transport/log/Log.hh>
#include <ignition/transport/log/Playback.hh>
#include <ignition/transport/log/Recorder.hh>
#include <ignition/transport/Node.hh>

#include "ChirpParams.hh"

struct MessageInformation
{
  public: std::string data;
  public: std::string type;
  public: std::string topic;
};

static std::mutex dataMutex;

//////////////////////////////////////////////////
/// \brief This is used within lambda callbacks to keep track of incoming
/// messages.
/// \param[out] _archive A vector that will store the incoming message
/// information. This must be passed from a lambda which has captured a vector.
/// \param[in] _data The data passed by the SubscribeRaw
/// \param[in] _len The length of data passed by the SubscribeRaw
/// \param[in] _msgInfo The metainfo about the message, provided by the
/// SubscribeRaw.
void TrackMessages(std::vector<MessageInformation> &_archive,
                   const char *_data,
                   std::size_t _len,
                   const ignition::transport::MessageInfo &_msgInfo)
{
  MessageInformation info;
  info.data = std::string(_data, _len);
  info.type = _msgInfo.Type();
  info.topic = _msgInfo.Topic();

  std::unique_lock<std::mutex> lock(dataMutex);
  _archive.push_back(info);
}


//////////////////////////////////////////////////
/// \brief Compare sent and received messages
/// \param[in] _recorded messages that were recorded
/// \param[in] _played messages that were published
void ExpectSameMessages(const std::vector<MessageInformation> &_recorded,
    const std::vector<MessageInformation> &_played)
{
  for (std::size_t i = 0; i < _recorded.size() && i < _played.size(); ++i)
  {
    const MessageInformation &original = _recorded[i];
    const MessageInformation &playedBack = _played[i];

    EXPECT_EQ(original.data, playedBack.data);
    EXPECT_EQ(original.type, playedBack.type);
    EXPECT_EQ(original.topic, playedBack.topic);
  }

  EXPECT_EQ(_recorded.size(), _played.size());
}


//////////////////////////////////////////////////
/// \brief Record a log and then play it back. Verify that the playback matches
/// the original.
TEST(playback, ReplayLog)
{
  std::vector<std::string> topics = {"/foo", "/bar", "/baz"};

  std::vector<MessageInformation> incomingData;

  auto callback = [&incomingData](
      const char *_data,
      std::size_t _len,
      const ignition::transport::MessageInfo &_msgInfo)
  {
    TrackMessages(incomingData, _data, _len, _msgInfo);
  };

  ignition::transport::Node node;
  ignition::transport::log::Recorder recorder;

  for (const std::string &topic : topics)
  {
    node.SubscribeRaw(topic, callback);
    recorder.AddTopic(topic);
  }

  const std::string logName = "file:playbackReplayLog?mode=memory&cache=shared";
  EXPECT_EQ(ignition::transport::log::RecorderError::SUCCESS,
    recorder.Start(logName));

  const int numChirps = 100;
  testing::forkHandlerType chirper =
    ignition::transport::log::test::BeginChirps(topics, numChirps);

  // Wait for the chirping to finish
  testing::waitAndCleanupFork(chirper);

  // Wait to make sure our callbacks are done processing the incoming messages
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Create playback before stopping so sqlite memory database is shared
  ignition::transport::log::Playback playback(logName);
  recorder.Stop();

  // Make a copy of the data so we can compare it later
  std::vector<MessageInformation> originalData = incomingData;

  // Clear out the old data so we can recreate it during the playback
  incomingData.clear();

  for (const std::string &topic : topics)
  {
    playback.AddTopic(topic);
  }

  playback.Start();

  std::cout << "Waiting to for playback to finish..." << std::endl;
  playback.WaitUntilFinished();
  std::cout << " Done waiting..." << std::endl;
  playback.Stop();
  std::cout << "Playback finished!" << std::endl;

  // Wait to make sure our callbacks are done processing the incoming messages
  // (Strangely, Windows throws an exception when this is ~1s or more)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ExpectSameMessages(originalData, incomingData);
}


//////////////////////////////////////////////////
TEST(playback, ReplayNoSuchTopic)
{
  ignition::transport::log::Recorder recorder;
  const std::string logName =
    "file:playbackReplayNoSuchTopic?mode=memory&cache=shared";
  EXPECT_EQ(ignition::transport::log::RecorderError::SUCCESS,
    recorder.Start(logName));

  ignition::transport::log::Playback playback(logName);
  recorder.Stop();

  EXPECT_EQ(ignition::transport::log::PlaybackError::NO_SUCH_TOPIC,
      playback.AddTopic("/DNE"));
  EXPECT_EQ(0, playback.AddTopic(std::regex("/DNE")));
}


//////////////////////////////////////////////////
/// \brief Record a log and then play it back. Verify that the playback matches
/// the original.
TEST(playback, ReplayLogRegex)
{
  std::vector<std::string> topics = {"/foo", "/bar", "/baz"};

  std::vector<MessageInformation> incomingData;

  auto callback = [&incomingData](
      const char *_data,
      std::size_t _len,
      const ignition::transport::MessageInfo &_msgInfo)
  {
    TrackMessages(incomingData, _data, _len, _msgInfo);
  };

  ignition::transport::Node node;
  ignition::transport::log::Recorder recorder;

  for (const std::string &topic : topics)
  {
    node.SubscribeRaw(topic, callback);
  }
  recorder.AddTopic(std::regex(".*"));

  const std::string logName =
    "file:playbackReplayLogRegex?mode=memory&cache=shared";
  EXPECT_EQ(ignition::transport::log::RecorderError::SUCCESS,
    recorder.Start(logName));

  const int numChirps = 100;
  testing::forkHandlerType chirper =
      ignition::transport::log::test::BeginChirps(topics, numChirps);

  // Wait for the chirping to finish
  testing::waitAndCleanupFork(chirper);

  // Wait to make sure our callbacks are done processing the incoming messages
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Create playback before stopping so sqlite memory database is shared
  ignition::transport::log::Playback playback(logName);
  recorder.Stop();

  // Make a copy of the data so we can compare it later
  std::vector<MessageInformation> originalData = incomingData;

  // Clear out the old data so we can recreate it during the playback
  incomingData.clear();

  playback.Start();
  std::cout << "Waiting to for playback to finish..." << std::endl;
  playback.WaitUntilFinished();
  std::cout << " Done waiting..." << std::endl;
  playback.Stop();
  std::cout << "Playback finished!" << std::endl;

  // Wait to make sure our callbacks are done processing the incoming messages
  // (Strangely, Windows throws an exception when this is ~1s or more)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ExpectSameMessages(originalData, incomingData);
}


//////////////////////////////////////////////////
/// \brief Record a log and then play it back. Verify that the playback matches
/// the original.
TEST(playback, ReplayLogMoveInstances)
{
  std::vector<std::string> topics = {"/foo", "/bar", "/baz"};

  std::vector<MessageInformation> incomingData;

  auto callback = [&incomingData](
      const char *_data,
      std::size_t _len,
      const ignition::transport::MessageInfo &_msgInfo)
  {
    TrackMessages(incomingData, _data, _len, _msgInfo);
  };

  ignition::transport::Node node;
  ignition::transport::log::Recorder recorder_orig;

  for (const std::string &topic : topics)
  {
    node.SubscribeRaw(topic, callback);
  }
  recorder_orig.AddTopic(std::regex(".*"));

  ignition::transport::log::Recorder recorder(std::move(recorder_orig));

  const std::string logName =
    "file:playbackReplayLogRegex?mode=memory&cache=shared";
  EXPECT_EQ(ignition::transport::log::RecorderError::SUCCESS,
    recorder.Start(logName));

  const int numChirps = 100;
  testing::forkHandlerType chirper =
      ignition::transport::log::test::BeginChirps(topics, numChirps);

  // Wait for the chirping to finish
  testing::waitAndCleanupFork(chirper);

  // Wait to make sure our callbacks are done processing the incoming messages
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Create playback before stopping so sqlite memory database is shared
  ignition::transport::log::Playback playback_orig(logName);
  recorder.Stop();

  // Make a copy of the data so we can compare it later
  std::vector<MessageInformation> originalData = incomingData;

  // Clear out the old data so we can recreate it during the playback
  incomingData.clear();

  playback_orig.AddTopic(std::regex(".*"));
  ignition::transport::log::Playback playback(std::move(playback_orig));
  playback.Start();

  std::cout << "Waiting to for playback to finish..." << std::endl;
  playback.WaitUntilFinished();
  std::cout << " Done waiting..." << std::endl;
  playback.Stop();
  std::cout << "Playback finished!" << std::endl;

  // Wait to make sure our callbacks are done processing the incoming messages
  // (Strangely, Windows throws an exception when this is ~1s or more)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ExpectSameMessages(originalData, incomingData);
}


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  setenv(ignition::transport::log::SchemaLocationEnvVar.c_str(),
         IGN_TRANSPORT_LOG_SQL_PATH, 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
