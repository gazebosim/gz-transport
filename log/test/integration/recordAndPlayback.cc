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

#include <gtest/gtest.h>

#include <ignition/transport/test_config.h>

#include <ignition/transport/log/Record.hh>
#include <ignition/transport/log/Log.hh>

#include "ChirpParams.hh"

//////////////////////////////////////////////////
testing::forkHandlerType BeginChirps(
    const std::vector<std::string> &_topics,
    const int _chirps)
{
  std::string args;

  // Add the partition name
  ignition::transport::env("IGN_PARTITION", args);

  // Add the number of chirps
  args += " " + std::to_string(_chirps);

  for (const std::string &topic : _topics)
  {
    args += " " + topic;
  }

  return testing::forkAndRun(
        IGN_TRANSPORT_LOG_BUILD_PATH"/topicChirp_aux", args.c_str());
}

//////////////////////////////////////////////////
TEST(recordAndPlayback, BeginRecordBeforeAdvertisement)
{
  std::vector<std::string> topics = {"foo", "bar"};

  ignition::transport::log::Record recorder;
  for (const std::string &topic : topics)
  {
    recorder.AddTopic(topic);
  }

  const std::string logName = IGN_TRANSPORT_LOG_BUILD_PATH"/test.log";

  EXPECT_EQ(recorder.Start(logName),
            ignition::transport::log::RecordError::NO_ERROR);

  const int numChirps = 10;
  testing::forkHandlerType fork = BeginChirps(topics, numChirps);

  // Wait for the chirping to finish
  testing::waitAndCleanupFork(fork);

  // Stop recording so we can safely view the log
  recorder.Stop();

  ignition::transport::log::Log log;
  EXPECT_TRUE(log.Open(logName));

  ignition::transport::log::MsgIter iter = log.AllMessages();

  std::string data;
  std::string type;
  // Get information for the last message that was received
  // TODO(MXG): Create a copy constructor for the Message class
  while (iter != ignition::transport::log::MsgIter())
  {
    data = iter->Data();
    type = iter->Type();
    ++iter;
  }

  EXPECT_FALSE(data.empty());
  EXPECT_FALSE(type.empty());

  using MsgType = ignition::transport::log::test::ChirpMsgType;

  EXPECT_EQ(MsgType().GetTypeName(), type);

  MsgType msg;
  EXPECT_TRUE(msg.ParseFromString(data));

  // The data of the last message should match the number of chirps that we
  // requested.
  EXPECT_EQ(numChirps, msg.data());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
