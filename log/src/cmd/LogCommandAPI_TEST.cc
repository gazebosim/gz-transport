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

#include "gtest/gtest.h"

static const std::string kGzLogCommand(std::string(GZ_PATH) + " log ");

/////////////////////////////////////////////////
std::string customExecStr(std::string _cmd)
{
  std::cout << "Running command [" << _cmd << "]" << std::endl;

  _cmd += " 2>&1";
  FILE *pipe = popen(_cmd.c_str(), "r");

  if (!pipe)
    return "ERROR";

  char buffer[128];
  std::string result = "";

  while (!feof(pipe))
  {
    if (fgets(buffer, 128, pipe) != nullptr)
    {
      result += buffer;
    }
  }

  pclose(pipe);
  return result;
}

//////////////////////////////////////////////////
// Test `gz log record` subcommand for regex
TEST(LogCommandAPI, RecordBadRegex)
{
  // Tested command: gz log record --file ':memory:' --pattern '*'
  const std::string cmd =
    kGzLogCommand + "record --file ':memory:' --pattern '*'";
  const std::string output = customExecStr(cmd);
  const std::string expectedOutput = "Regex pattern is invalid\n";

  EXPECT_EQ(expectedOutput, output);
}

//////////////////////////////////////////////////
// Test `gz log playback` subcommand for regex
TEST(LogCommandAPI, PlaybackBadRegex)
{
  // Tested command: gz log record --file ':memory:' --pattern '*' --wait 0
  const std::string cmd =
    kGzLogCommand + "playback --file ':memory:' --pattern '*' "
    "--wait 0";
  const std::string output = customExecStr(cmd);
  const std::string expectedOutput = "Regex pattern is invalid\n";

  EXPECT_EQ(expectedOutput, output);
}

//////////////////////////////////////////////////
// Test `gz log playback` subcommand for topic remap
TEST(LogCommandAPI, PlaybackBadRemap)
{
  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*' --wait 0 --remap '/foo' -f
  {
    const std::string cmd =
      kGzLogCommand + "playback --file ':memory:' --pattern '.*' "
      "--wait 0 --remap '/foo' -f";
    const std::string output = customExecStr(cmd);
    const std::string expectedOutput =
      "Invalid remap as := delimiter is missing\n";

    EXPECT_EQ(expectedOutput, output);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*' --wait 0 --remap '/foo:='
  {
    const std::string cmd =
      kGzLogCommand + "playback --file ':memory:' --pattern '.*' "
      "--wait 0 --remap '/foo:='";
    const std::string output = customExecStr(cmd);
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_EQ(expectedOutput, output);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*'
  // --wait 0 --remap '/foo:=' -f
  {
    const std::string cmd =
      kGzLogCommand + "playback --file ':memory:' --pattern '.*' "
      "--wait 0 --remap '/foo:=' -f";
    const std::string output = customExecStr(cmd);
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_EQ(expectedOutput, output);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*' --wait 0 --remap ':=/bar'
  {
    const std::string cmd =
      kGzLogCommand + "playback --file ':memory:' --pattern '.*' "
      "--wait 0 --remap ':=/bar'";
    const std::string output = customExecStr(cmd);
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_EQ(expectedOutput, output);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*'
  // --wait 0 --remap ':=/bar' -f
  {
    const std::string cmd =
      kGzLogCommand + "playback --file ':memory:' --pattern '.*' "
      "--wait 0 --remap ':=/bar' -f";
    const std::string output = customExecStr(cmd);
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_EQ(expectedOutput, output);
  }
}

//////////////////////////////////////////////////
// Test `gz log record` subcommand for opening file
TEST(LogCommandAPI, RecordFailedToOpen)
{
  // Tested command: gz log record --file '!@#$%^&*(:;[{]})?/.|' --pattern '.*'
  const std::string cmd =
    kGzLogCommand + "record --file '!@#$%^&*(:;[{]})?/.|' --pattern '.*'";
  const std::string output = customExecStr(cmd);
  const std::string expectedOutput =
    "Failed to open the requested sqlite3 database\n"
    "Failed to open or create file [!@#$%^&*(:;[{]})?/.|]\n";

  EXPECT_EQ(expectedOutput, output);
}

//////////////////////////////////////////////////
// Test `gz log playback` subcommand for opening file
TEST(LogCommandAPI, PlaybackFailedToOpen)
{
  // Tested command:
  // gz log playback --file '!@#$%^&*(:;[{]})?/.|' --pattern '.*' --wait 0
  const std::string cmd =
    kGzLogCommand + "playback --file '!@#$%^&*(:;[{]})?/.|' "
    "--pattern '.*' --wait 0";
  const std::string output = customExecStr(cmd);
  const std::string expectedOutput =
    "Failed to open the requested sqlite3 database\n"
    "Could not open file [!@#$%^&*(:;[{]})?/.|]\n";

  EXPECT_EQ(expectedOutput, output);
}
