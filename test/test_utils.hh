/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef GZ_TRANSPORT_TEST_UTILS_HH_
#define GZ_TRANSPORT_TEST_UTILS_HH_

#include "gtest/gtest.h"

#include <climits>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

namespace testing
{
  /// \brief Get a random number based on an integer converted to string.
  /// \return A random integer converted to string.
  inline std::string getRandomNumber()
  {
    // Initialize random number generator.
    uint32_t seed = std::random_device {}();
    std::mt19937 randGenerator(seed);

    // Create a random number based on an integer converted to string.
    std::uniform_int_distribution<int32_t> d(0, INT_MAX);

    return std::to_string(d(randGenerator));
  }

  class PartitionedTransportTest: public ::testing::Test
  {
    protected: void SetUp() override {
      gz::utils::env("GZ_PARTITION", this->prevPartition);

      // Get a random partition name.
      this->partition = testing::getRandomNumber();

      // Set the partition name for this process.
      gz::utils::setenv("GZ_PARTITION", this->partition);
    }

    protected: void TearDown() override {
      gz::utils::setenv("GZ_PARTITION", this->prevPartition);

      if (this->pi)
      {
        this->pi->Terminate();
        this->pi->Join();
      }
    }

    protected: [[nodiscard]] std::string Paritition() const {
      return this->partition;
    }

    protected: void SpawnSubprocess(
      const std::vector<std::string> &_commandLine,
      const gz::utils::EnvironmentMap &_overrideEnv={})
    {
      auto environment = gz::utils::env();
      for (const auto &[k, v] : _overrideEnv)
        environment[k] = v;
      this->pi =
        std::make_unique<gz::utils::Subprocess>(_commandLine, environment);
    }

    private: std::string prevPartition;
    private: std::string partition;
    private: std::unique_ptr<gz::utils::Subprocess> pi;
  };
}  // namespace testing

#endif  // GZ_TRANSPORT_TEST_UTILS_HH_
