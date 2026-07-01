/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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
#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>
#include "gz/utils/Subprocess.hh"

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static constexpr const char *g_topic = "/foo";
static constexpr int g_data = 5;

//////////////////////////////////////////////////
/// \brief Fixture that does NOT spawn the responder in SetUp.
/// The test body opens a Node first and only then spawns the
/// replier, exercising the cold-start path where a request is
/// issued before the responder's queryable has been declared.
class twoProcSrvCallLateResponder : public testing::Test {
 protected:
  void SetUp() override {
    gz::utils::env("GZ_PARTITION", this->prevPartition);
    this->partition = testing::getRandomNumber();
    gz::utils::setenv("GZ_PARTITION", this->partition);
    // Note: do NOT spawn the replier yet.
  }

  void TearDown() override {
    if (this->replier)
    {
      this->replier->Terminate();
      this->replier->Join();
    }
    gz::utils::setenv("GZ_PARTITION", this->prevPartition);
  }

  /// \brief Spawn the replier. Typically called from a worker
  /// thread after the requester has issued a blocking Request.
  void SpawnReplier() {
    this->replier = std::make_unique<gz::utils::Subprocess>(
      std::vector<std::string>(
        {test_executables::kTwoProcsSrvCallReplier, this->partition}));
  }

 private:
  std::string prevPartition;
  std::string partition;
  std::unique_ptr<gz::utils::Subprocess> replier;
};

//////////////////////////////////////////////////
TEST_F(twoProcSrvCallLateResponder, RequestSucceedsWhenResponderAppearsLate)
{
  // Open the requester Node before the responder process exists.
  // The persistent Querier declares interest on /foo immediately;
  // once the responder is spawned, it sees the interest and
  // propagates its queryable so the in-flight request finds it.
  transport::Node node;

  // Spawn the replier from a worker thread 500 ms after we begin
  // the synchronous Request below. 500 ms is well inside the 5 s
  // request timeout, so a working request/reply path should
  // comfortably succeed within budget.
  std::thread spawner([this]()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    this->SpawnReplier();
  });

  msgs::Int32 req;
  req.set_data(g_data);
  msgs::Int32 rep;
  bool result = false;
  const unsigned int timeoutMs = 5000;

  const bool executed = node.Request(g_topic, req, timeoutMs, rep, result);

  if (spawner.joinable())
    spawner.join();

  EXPECT_TRUE(executed)
    << "Request did not complete within " << timeoutMs << "ms after the "
    << "late responder start";
  EXPECT_TRUE(result) << "responder returned a failure result";
  EXPECT_EQ(rep.data(), g_data);
}
