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
#include <memory>
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

  /// \brief Test fixture for creating a transport test in a new partition
  ///
  /// Each unit test instance will create a random partition and set the
  /// appropriate environment variables.
  /// This fixture will additionally terminate a process spawned with
  /// the SpawnSubprocess helper function
  class PartitionedTransportTest: public ::testing::Test
  {
    /// \brief Set up the test fixture
    protected: void SetUp() override {
      gz::utils::env("GZ_PARTITION", this->prevPartition);

      // Get a random partition name.
      this->partition = testing::getRandomNumber();

      // Set the partition name for this process.
      gz::utils::setenv("GZ_PARTITION", this->partition);
    }

    /// \brief Clean up the test fixture
    protected: void TearDown() override {
      gz::utils::setenv("GZ_PARTITION", this->prevPartition);

      if (this->pi)
      {
        this->pi->Terminate();
        this->pi->Join();
      }
    }

    /// \brief Get the randomly generated partition for this test
    /// \return string value of the partition
    protected: [[nodiscard]] std::string Partition() const {
      return this->partition;
    }

    /// \brief Spawn a subprocess that will be terminated when the test fixture
    /// is torn down.  By default, the subprocess will inherit the environment
    /// of the test fixture. The _overrideEnv argument can be used to replace
    /// environment variable values.
    ///
    /// \param[in] _commandLine command line arguments
    /// \param[in] _overrideEnv environment variables to override
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
