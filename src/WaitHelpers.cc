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

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#else
  #include <unistd.h>
#endif

#include "gz/transport/WaitHelpers.hh"
#include "gz/transport/Node.hh"

namespace gz::transport
{
inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
{
namespace
{
// Platform shims for pipe / read / write. On Windows the C runtime
// provides POSIX-compatible _pipe / _read / _write in <io.h>.
#ifdef _WIN32
  inline int gzPipe(int _fds[2])
  { return _pipe(_fds, 256, _O_BINARY); }
  inline int gzRead(int _fd, void *_buf, unsigned int _n)
  { return _read(_fd, _buf, _n); }
  inline int gzWrite(int _fd, const void *_buf, unsigned int _n)
  { return _write(_fd, _buf, _n); }
#else
  inline int gzPipe(int _fds[2])
  { return ::pipe(_fds); }
  inline ssize_t gzRead(int _fd, void *_buf, size_t _n)
  { return ::read(_fd, _buf, _n); }
  inline ssize_t gzWrite(int _fd, const void *_buf, size_t _n)
  { return ::write(_fd, _buf, _n); }
#endif

/// \brief Self-pipe used to wake waitForShutdown() from the signal handler.
/// The signal handler writes one byte (async-signal-safe on POSIX, a normal
/// thread-safe kernel call on Windows). waitForShutdown() blocks on read().
int g_shutdownPipe[2] = {-1, -1};
}  // namespace

//////////////////////////////////////////////////
/// \brief Function executed when a SIGINT or SIGTERM signal is captured.
/// Only async-signal-safe operations are used here: per signal-safety(7),
/// write() is on the POSIX async-signal-safe list, while mutex and condition
/// variable operations are not.
/// \param[in] _signal Signal received.
static void signal_handler(const int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
  {
    if (g_shutdownPipe[1] < 0)
      return;
    const char c = 'x';
    // On Windows the handler runs on a runtime-spawned thread, so this
    // is just a thread-safe kernel call. Ignore short writes / EAGAIN —
    // a single byte is sufficient to wake the reader.
    auto n = gzWrite(g_shutdownPipe[1], &c, 1);
    (void)n;
  }
}

//////////////////////////////////////////////////
void waitForShutdown()
{
  // Lazily create the self-pipe on first call. We never close the fds:
  // waitForShutdown() is invoked at most once per process in practice,
  // and the OS reclaims them at exit.
  if (g_shutdownPipe[0] < 0 && gzPipe(g_shutdownPipe) != 0)
    return;

  // Install handlers AFTER the pipe exists so a signal arriving between
  // the two calls cannot find an uninitialized pipe.
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  // Block until the signal handler writes one byte. Retry on EINTR
  // because the signal itself may interrupt the read().
  char c;
  while (true)
  {
    auto n = gzRead(g_shutdownPipe[0], &c, 1);
    if (n == 1)
      break;
    if (n < 0 && errno == EINTR)
      continue;
    break;  // EOF or unrecoverable error
  }
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
