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

// Regression tests for tearing down Zenoh entities from within
// their own callbacks.
//
// zenoh-c's z_undeclare_subscriber() and z_undeclare_queryable()
// call wait_callbacks(), which blocks until the entity's callback
// closure is released. Because the handler lambdas capture a
// weak_ptr and lock it for the duration of the dispatch, the LAST
// handler reference routinely dies inside the entity's own
// callback (e.g. a service callback that unadvertises its own
// service). If the handler destructor undeclared inline, the Zenoh
// worker thread would park waiting for its own invocation to
// return: the process still looks healthy, replies already sent
// are delivered and exit is clean, but the thread never dispatches
// again. gz-transport therefore defers the undeclare to a separate
// thread (see ZenohTeardownEntity in NodeSharedPrivate.hh); these
// tests fail if that deferral is ever removed.

#include <gz/msgs/int32.pb.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gz/transport/Node.hh"
#include "gz/transport/WaitHelpers.hh"

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static std::atomic<int> g_cbCount{0};
static std::atomic<bool> g_cbEntered{false};

//////////////////////////////////////////////////
/// \brief A remote responder unadvertises its own service from
/// inside the service callback, then must still be able to answer
/// a second service. If the queryable teardown runs inline on the
/// callback thread, the second request times out and the auxiliary
/// process exits with a non-zero code.
TEST(zenohTeardownFromCallback, UnadvertiseSrvFromOwnCallback)
{
  std::string prevPartition;
  gz::utils::env("GZ_PARTITION", prevPartition);
  const std::string partition = testing::getRandomNumber();
  gz::utils::setenv("GZ_PARTITION", partition);

  gz::utils::Subprocess aux(
    std::vector<std::string>(
      {test_executables::kZenohTeardownFromCallbackAux, partition}));

  transport::Node node;
  ASSERT_TRUE(transport::waitForService(node, "/teardown_cb_srv"))
    << "responder service never discovered";

  msgs::Int32 req;
  req.set_data(7);
  msgs::Int32 rep;
  bool result = false;
  const bool executed = node.Request("/teardown_cb_srv", req, 5000u,
                                     rep, result);
  EXPECT_TRUE(executed) << "first request never completed";

  // Liveness: the aux destroyed its first queryable on a Zenoh
  // callback thread. A second service in the same process must
  // still answer; if not, that thread parked in an inline
  // undeclare.
  // Discovery still works while a worker is parked (the liveliness
  // token is declared from the aux main thread); only the query
  // dispatch dies. Waiting for discovery first keeps the failure
  // signal unambiguous: a timeout below means dispatch, not
  // discovery.
  EXPECT_TRUE(transport::waitForService(node, "/teardown_cb_srv2"))
    << "second service never discovered";
  msgs::Int32 rep2;
  bool result2 = false;
  const bool executed2 = node.Request("/teardown_cb_srv2", req, 5000u,
                                      rep2, result2);
  EXPECT_TRUE(executed2)
    << "second request timed out: the Zenoh callback thread likely "
    << "parked in an inline undeclare during the in-callback teardown";
  if (executed2)
    EXPECT_EQ(rep2.data(), req.data() + 1);

  const int rc = aux.Join();
  EXPECT_EQ(0, rc) << "aux rc=" << rc
                   << " (2: callback never ran, 4: second service "
                   << "never called)";

  gz::utils::setenv("GZ_PARTITION", prevPartition);
}

//////////////////////////////////////////////////
/// \brief Repeatedly destroy a subscription while its callback is
/// executing on a Zenoh thread, then verify that callbacks still
/// flow. Guards the same invariant on the subscriber side, and the
/// churn catches gradual worker-thread exhaustion.
TEST(zenohTeardownFromCallback, UnsubscribeWhileCallbackInFlight)
{
  std::string prevPartition;
  gz::utils::env("GZ_PARTITION", prevPartition);
  const std::string partition = testing::getRandomNumber();
  gz::utils::setenv("GZ_PARTITION", partition);

  {
    transport::Node pubNode;
    auto pub = pubNode.Advertise<msgs::Int32>("/teardown_cb_topic");

    constexpr int kChurn = 8;
    int churned = 0;
    for (int i = 0; i < kChurn; ++i)
    {
      g_cbEntered = false;
      auto *subNode = new transport::Node();
      std::function<void(const msgs::Int32 &)> cb =
        [](const msgs::Int32 &)
        {
          g_cbEntered = true;
          // Long-running user callback on the Zenoh thread.
          std::this_thread::sleep_for(std::chrono::milliseconds(600));
          ++g_cbCount;
        };
      ASSERT_TRUE(subNode->Subscribe("/teardown_cb_topic", cb));

      // Publish until the callback is running on the Zenoh thread
      // (repeating absorbs discovery latency).
      msgs::Int32 msg;
      msg.set_data(1);
      ASSERT_TRUE(transport::waitUntil([&pub, &msg]
        {
          pub.Publish(msg);
          return g_cbEntered.load();
        }, std::chrono::seconds(10), std::chrono::milliseconds(100)))
        << "iteration " << i << ": callback never fired "
        << "(Zenoh workers exhausted?)";

      // Tear down the subscriber node while the callback sleeps:
      // the receive lambda holds the last handler reference, so the
      // handler dies on the Zenoh thread when the callback returns.
      delete subNode;
      ++churned;
    }
    EXPECT_EQ(kChurn, churned);

    // Let in-flight callbacks and deferred teardown settle.
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Liveness: a fresh round trip must still work.
    std::atomic<bool> alive{false};
    transport::Node checkNode;
    std::function<void(const msgs::Int32 &)> aliveCb =
      [&alive](const msgs::Int32 &) { alive = true; };
    ASSERT_TRUE(checkNode.Subscribe("/teardown_cb_alive", aliveCb));
    auto alivePub = pubNode.Advertise<msgs::Int32>("/teardown_cb_alive");
    msgs::Int32 ping;
    ping.set_data(2);
    EXPECT_TRUE(transport::waitUntil([&alivePub, &ping, &alive]
      {
        alivePub.Publish(ping);
        return alive.load();
      }, std::chrono::seconds(5), std::chrono::milliseconds(100)))
      << "Zenoh stopped delivering callbacks after the teardown "
      << "churn: worker threads likely parked in inline undeclare";
  }

  gz::utils::setenv("GZ_PARTITION", prevPartition);
}
