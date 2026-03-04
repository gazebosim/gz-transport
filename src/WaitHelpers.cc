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

#include <algorithm>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <vector>

#include "gz/transport/WaitHelpers.hh"
#include "gz/transport/Node.hh"

namespace gz::transport
{
inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
{
/// \brief Flag to detect SIGINT or SIGTERM while the code is executing
/// waitForShutdown().
static bool g_shutdown = false;

/// \brief Mutex to protect the boolean shutdown variable.
static std::mutex g_shutdown_mutex;

/// \brief Condition variable to wakeup waitForShutdown() and exit.
static std::condition_variable g_shutdown_cv;

//////////////////////////////////////////////////
/// \brief Function executed when a SIGINT or SIGTERM signals are captured.
/// \param[in] _signal Signal received.
static void signal_handler(const int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
  {
    g_shutdown_mutex.lock();
    g_shutdown = true;
    g_shutdown_mutex.unlock();
    g_shutdown_cv.notify_all();
  }
}

//////////////////////////////////////////////////
void waitForShutdown()
{
  // Install a signal handler for SIGINT and SIGTERM.
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::unique_lock<std::mutex> lk(g_shutdown_mutex);
  g_shutdown_cv.wait(lk, []{return g_shutdown;});
}

//////////////////////////////////////////////////
bool waitForService(
    const Node &_node,
    const std::string &_service,
    std::chrono::milliseconds _timeout)
{
  return waitUntil([&]() {
    std::vector<ServicePublisher> publishers;
    _node.ServiceInfo(_service, publishers);
    return !publishers.empty();
  }, _timeout, std::chrono::milliseconds(100));
}

//////////////////////////////////////////////////
bool waitForTopic(
    const Node &_node,
    const std::string &_topic,
    std::chrono::milliseconds _timeout)
{
  return waitUntil([&]() {
    std::vector<std::string> topics;
    _node.TopicList(topics);
    return std::find(topics.begin(), topics.end(), _topic) != topics.end();
  }, _timeout, std::chrono::milliseconds(100));
}
}  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport
