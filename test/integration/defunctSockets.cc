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

#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include <ignition/msgs.hh>

#include "gtest/gtest.h"
#include "gz/transport/Node.hh"
#include "gz/transport/test_config.h"

// Private in xnu, but exported by libsystem_kernel.
extern "C" int pid_shutdown_sockets(int _pid, int _level);

using namespace gz;
using namespace std::chrono_literals;

//////////////////////////////////////////////////
/// \brief When macOS runs out of network buffers, it defuncts every socket of
/// the process holding the most. The node must neither die of SIGPIPE nor
/// spin on its dead discovery sockets.
TEST(DefunctSockets, NodeSurvives)
{
  const std::string partition = testing::getRandomNumber();
  const pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0)
  {
    // Discovery reports each failed receive; keep that out of the test log.
    std::cerr.rdbuf(nullptr);
    setenv("IGN_PARTITION", partition.c_str(), 1);
    transport::Node node;
    auto pub = node.Advertise<msgs::Int32>("/defunct_sockets");
    std::this_thread::sleep_for(1s);

    // 2 is SHUTDOWN_SOCKET_LEVEL_DISCONNECT_ALL from xnu's sys/proc.h.
    if (pid_shutdown_sockets(getpid(), 2) != 0)
      _exit(2);
    // Several heartbeats go out on the dead sockets in 3 s.
    const std::clock_t start = std::clock();
    std::this_thread::sleep_for(3s);
    const double cpu = 1.0 * (std::clock() - start) / CLOCKS_PER_SEC;
    // _exit() skips the teardown, which can't work with dead sockets.
    _exit(cpu < 0.5 ? 0 : 1);
  }

  int status = 0;
  ASSERT_EQ(pid, waitpid(pid, &status, 0));
  ASSERT_FALSE(WIFSIGNALED(status)) << "Died of signal " << WTERMSIG(status);
  if (WEXITSTATUS(status) == 2)
  {
    // This googletest version predates GTEST_SKIP.
    std::cout << "pid_shutdown_sockets isn't permitted here, skipping"
              << std::endl;
    return;
  }
  EXPECT_EQ(0, WEXITSTATUS(status)) << "Used more than 0.5 s of CPU in 3 s";
}
