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

/// \brief Example of recording all ignition transport topics.
/// This will record all topics and currently published to a file.
/// Launch the ignition-transport publisher example so this example has
/// something to record.

#include <signal.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <regex>
#include <thread>
#include <condition_variable>
#include <ignition/transport/log/Recorder.hh>

using namespace std::chrono_literals;

static bool g_stop = false;
static std::condition_variable g_wakeup;

void Interrupt(int _signal)
{
  if (SIGINT == _signal)
  {
    g_stop = true;
    g_wakeup.notify_all();
  }
}

//////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " OUTPUT.tlog\n";
    return -1;
  }

  signal(SIGINT, Interrupt);

  ignition::transport::log::Recorder recorder;

  // Record all topics
  recorder.AddTopic(std::regex(".*"));

  // Begin recording, saving received messages to the given file
  auto result = recorder.Start(argv[1]);
  if (ignition::transport::log::RecorderError::SUCCESS != result)
  {
    std::cerr << "Failed to start recording: " << static_cast<int64_t>(result)
              << "\n";
    return -1;
  }

  std::cout << "Press Ctrl+C to finish recording.\n  Recording... "
            << std::endl;

  // Wait until the interrupt signal is sent. Waiting on a condition variable
  // prevents the process from eating up unnecessary CPU cycles.
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  g_wakeup.wait(lock, [&](){ return g_stop; });

  recorder.Stop();

  std::cout << "\nRecording finished!" << std::endl;
}
