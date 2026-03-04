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

#ifndef GZ_TRANSPORT_WAITHELPERS_HH_
#define GZ_TRANSPORT_WAITHELPERS_HH_

#include <chrono>
#include <string>
#include <thread>

#include "gz/transport/config.hh"
#include "gz/transport/Export.hh"

namespace gz::transport
{
  // Inline bracket to help doxygen filtering.
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE {
  //
  // Forward declaration.
  class Node;

  /// \brief Block the current thread until a SIGINT or SIGTERM is received.
  /// Note that this function registers a signal handler. Do not use this
  /// function if you want to manage yourself SIGINT/SIGTERM.
  void GZ_TRANSPORT_VISIBLE waitForShutdown();

  /// \brief Block until a predicate becomes true or a timeout expires.
  /// Polls by calling _pred() repeatedly with _interval sleep between
  /// checks. Uses steady_clock for monotonic timing.
  /// \note An _interval of zero causes busy-waiting. Use a positive
  /// interval in production code.
  /// \param[in] _pred Callable returning bool. Checked repeatedly.
  /// \param[in] _timeout Maximum time to wait (default 10 s).
  /// \param[in] _interval Sleep between checks (default 10 ms).
  /// \return True if the predicate became true, false on timeout.
  template <typename Pred>
  bool waitUntil(Pred _pred,
                 std::chrono::milliseconds _timeout =
                     std::chrono::milliseconds(10000),
                 std::chrono::milliseconds _interval =
                     std::chrono::milliseconds(10))
  {
    auto deadline = std::chrono::steady_clock::now() + _timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
      if (_pred())
        return true;
      std::this_thread::sleep_for(_interval);
    }
    return _pred();
  }

  /// \brief Block until a service is discovered or a timeout expires.
  /// \param[in] _node Node to query for service info.
  /// \param[in] _service Fully-qualified service name.
  /// \param[in] _timeout Maximum time to wait (default 10 s).
  /// \return True if the service was discovered, false on timeout.
  bool GZ_TRANSPORT_VISIBLE waitForService(
      const Node &_node,
      const std::string &_service,
      std::chrono::milliseconds _timeout =
          std::chrono::milliseconds(10000));

  /// \brief Block until a topic is discovered or a timeout expires.
  /// \param[in] _node Node to query for topic list.
  /// \param[in] _topic Fully-qualified topic name.
  /// \param[in] _timeout Maximum time to wait (default 10 s).
  /// \return True if the topic was discovered, false on timeout.
  bool GZ_TRANSPORT_VISIBLE waitForTopic(
      const Node &_node,
      const std::string &_topic,
      std::chrono::milliseconds _timeout =
          std::chrono::milliseconds(10000));
  }
}

// GZ_TRANSPORT_WAITHELPERS_HH_
#endif
