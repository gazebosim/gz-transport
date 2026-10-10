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

#include <gz/msgs/stringmsg.pb.h>

#include <atomic>
#include <chrono>
#include <regex>
#include <string>
#include <thread>

#include "gz/transport/Node.hh"
#include "gz/transport/NodeOptions.hh"
#include "gz/transport/log/Recorder.hh"
#include "gtest/gtest.h"

using namespace gz;

//////////////////////////////////////////////////
TEST(Record, Start)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(
      transport::log::RecorderError::SUCCESS, recorder.Start(":memory:"));
}

//////////////////////////////////////////////////
TEST(Record, StartImpossibleFilename)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(transport::log::RecorderError::FAILED_TO_OPEN,
      recorder.Start("//////////"));
}

//////////////////////////////////////////////////
TEST(Record, DoubleStart)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(
      transport::log::RecorderError::SUCCESS, recorder.Start(":memory:"));
  EXPECT_EQ(transport::log::RecorderError::ALREADY_RECORDING,
      recorder.Start(":memory:"));
}

//////////////////////////////////////////////////
TEST(Record, StartStopStart)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(
      transport::log::RecorderError::SUCCESS, recorder.Start(":memory:"));
  recorder.Stop();
  EXPECT_EQ(
      transport::log::RecorderError::SUCCESS, recorder.Start(":memory:"));
}

//////////////////////////////////////////////////
TEST(Record, AddValidTopic)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(0u, recorder.Topics().size());
  EXPECT_EQ(transport::log::RecorderError::SUCCESS,
      recorder.AddTopic(std::string("/foo")));
  EXPECT_EQ(transport::log::RecorderError::ALREADY_SUBSCRIBED_TO_TOPIC,
      recorder.AddTopic(std::string("/foo")));
  EXPECT_EQ(1u, recorder.Topics().size());
  EXPECT_NE(recorder.Topics().end(), recorder.Topics().find("/foo"));
}

//////////////////////////////////////////////////
TEST(Record, AddInvalidTopic)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(transport::log::RecorderError::FAILED_TO_SUBSCRIBE,
      recorder.AddTopic(std::string("/////")));
  EXPECT_EQ(0u, recorder.Topics().size());
}

//////////////////////////////////////////////////
TEST(Record, AddTopicRegex)
{
  transport::log::Recorder recorder;
  EXPECT_EQ(0, recorder.AddTopic(std::regex("////")));
}

//////////////////////////////////////////////////
TEST(Record, SetBufferSize)
{
  transport::log::Recorder recorder;
  recorder.SetBufferSize(20);
  EXPECT_EQ(20u, recorder.BufferSize());

  recorder.SetBufferSize(0);
  EXPECT_EQ(0u, recorder.BufferSize());

  recorder.SetBufferSize(40);
  EXPECT_EQ(40u, recorder.BufferSize());
}

//////////////////////////////////////////////////
/// \brief Destroy Recorders while liveliness announcements keep arriving.
/// Regression: the Recorder owns a discovery object whose callback used
/// to run into the destroyed Recorder, because zenoh-c before 1.8.0
/// undeclares a subscriber without waiting for a callback in flight.
TEST(Record, DestroyUnderDiscoveryChurn)
{
  std::atomic<bool> stop{false};
  std::thread churn([&stop]()
  {
    // A different partition, so the Recorders under test never subscribe
    // to these topics; their discovery callback still runs for each one.
    transport::NodeOptions opts;
    opts.SetPartition("recorder_churn");
    while (!stop)
    {
      transport::Node node(opts);
      for (int i = 0; i < 4; ++i)
      {
        node.Advertise<gz::msgs::StringMsg>(
          "/churn/topic" + std::to_string(i));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  });

  for (int i = 0; i < 50; ++i)
  {
    transport::log::Recorder recorder;
  }

  stop = true;
  churn.join();
}
