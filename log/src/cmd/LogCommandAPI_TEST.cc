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

#include <string>
#include <vector>

#include <gz/utils/Subprocess.hh>

static const std::string kGzLogCommand(std::string(GZ_PATH));

/////////////////////////////////////////////////
struct ProcessOutput
{
  int code{-1};
  std::string cout;
  std::string cerr;
};

/////////////////////////////////////////////////
ProcessOutput customExecStr(const std::vector<std::string> &_args)
{
  auto fullArgs = std::vector<std::string>{kGzLogCommand, "log"};
  std::copy(std::begin(_args), std::end(_args), std::back_inserter(fullArgs));
  auto proc = gz::utils::Subprocess(fullArgs);
  auto return_code = proc.Join();
  return {return_code, proc.Stdout(), proc.Stderr()};
}

//////////////////////////////////////////////////
// Test `gz log record` subcommand for regex
TEST(LogCommandAPI, RecordBadRegex)
{
  // Tested command: gz log record --file ':memory:' --pattern '*'
  auto output = customExecStr({
      "record",
      "--file", ":memory:",
      "--pattern", "*"});
  const std::string expectedOutput = "Regex pattern is invalid\n";

  EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
}

//////////////////////////////////////////////////
// Test `gz log playback` subcommand for regex
TEST(LogCommandAPI, PlaybackBadRegex)
{
  // Tested command: gz log record --file ':memory:' --pattern '*' --wait 0
  auto output = customExecStr({
      "playback",
      "--file", ":memory:",
      "--pattern", "*",
      "--wait", "0"});
  const std::string expectedOutput = "Regex pattern is invalid\n";

  EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
}

//////////////////////////////////////////////////
// Test `gz log playback` subcommand for topic remap
TEST(LogCommandAPI, PlaybackBadRemap)
{
  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*' --wait 0 --remap '/foo' -f
  {
    auto output = customExecStr({
        "playback",
        "--file", ":memory:",
        "--pattern", ".*",
        "--wait", "0",
        "--remap", "/foo",
        "-f"});
    const std::string expectedOutput =
      "Invalid remap as := delimiter is missing\n";

    EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*' --wait 0 --remap '/foo:='
  {
    auto output = customExecStr({
        "playback",
        "--file", ":memory:",
        "--pattern", ".*",
        "--wait", "0",
        "--remap", "/foo:="});
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*'
  // --wait 0 --remap '/foo:=' -f
  {
    auto output = customExecStr({
        "playback",
        "--file", ":memory:",
        "--pattern", ".*",
        "--wait", "0",
        "--remap", "/foo:=",
        "-f"});
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*' --wait 0 --remap ':=/bar'
  {
    auto output = customExecStr({
        "playback",
        "--file", ":memory:",
        "--pattern", ".*",
        "--wait", "0",
        "--remap", ":=/bar"});
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
  }

  // Tested command:
  // gz log playback --file ':memory:' --pattern '.*'
  // --wait 0 --remap ':=/bar' -f
  {
    auto output = customExecStr({
        "playback",
        "--file", ":memory:",
        "--pattern", ".*",
        "--wait", "0",
        "--remap", ":=/bar",
        "-f"});
    const std::string expectedOutput =
      "Invalid topic name []\n"
      "Invalid remap of topics\n";

    EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
  }
}

//////////////////////////////////////////////////
// Test `gz log record` subcommand for opening file
TEST(LogCommandAPI, RecordFailedToOpen)
{
  // Tested command: gz log record --file '!@#$%^&*(:;[{]})?/.|' --pattern '.*'
  auto output = customExecStr({
      "record",
      "--file", "!@#$%^&*(:;[{]})?/.|",
      "--pattern", ".*"});
  const std::string expectedOutput =
    "Failed to open the requested sqlite3 database\n"
    "Failed to open or create file [!@#$%^&*(:;[{]})?/.|]\n";

  EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
}

//////////////////////////////////////////////////
// Test `gz log playback` subcommand for opening file
TEST(LogCommandAPI, PlaybackFailedToOpen)
{
  // Tested command:
  // gz log playback --file '!@#$%^&*(:;[{]})?/.|' --pattern '.*' --wait 0
  auto output = customExecStr({
      "playback",
      "--file", "!@#$%^&*(:;[{]})?/.|",
      "--pattern", ".*",
      "--wait", "0"});
  const std::string expectedOutput =
    "Failed to open the requested sqlite3 database\n"
    "Could not open file [!@#$%^&*(:;[{]})?/.|]\n";

  EXPECT_NE(output.cerr.find(expectedOutput), std::string::npos);
}
