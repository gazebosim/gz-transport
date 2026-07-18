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
#include "gtest/gtest.h"

#include <gz/msgs/int32.pb.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#ifndef _WIN32
  #include <pthread.h>
  #include <unistd.h>
#endif

#include "gz/transport/WaitHelpers.hh"
#include "gz/transport/Node.hh"
#include "test_utils.hh"

#include <gz/utils/Environment.hh>

using namespace gz;

#ifndef _WIN32
namespace
{
//////////////////////////////////////////////////
void NoopSignalHandler(int)
{
}

//////////////////////////////////////////////////
void VerifyWaitForShutdownBurstSignalsStayNonBlocking()
{
  std::thread waiter([] { transport::waitForShutdown(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  waiter.join();

  std::atomic<int> signalCount{0};
  std::atomic<bool> finished{false};
  std::thread signalThread([&]
  {
    for (int i = 0; i < 200000; ++i)
    {
      raise(SIGINT);
      signalCount.store(i + 1);
    }
    finished.store(true);
  });

  int lastCount = 0;
  int stalledChecks = 0;
  while (!finished.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const int currentCount = signalCount.load();
    if (currentCount == lastCount)
    {
      ++stalledChecks;
      if (stalledChecks >= 10)
        _exit(1);
    }
    else
    {
      stalledChecks = 0;
      lastCount = currentCount;
    }
  }

  signalThread.join();
  _exit(0);
}

//////////////////////////////////////////////////
void VerifyWaitForShutdownPreservesErrno()
{
  struct sigaction action = {};
  action.sa_handler = NoopSignalHandler;
  sigemptyset(&action.sa_mask);
  sigaction(SIGUSR1, &action, nullptr);

  std::thread waiter([] { transport::waitForShutdown(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  waiter.join();

  std::atomic<int> signalCount{0};
  std::atomic<bool> finished{false};
  std::atomic<bool> stopRequested{false};
  std::atomic<int> observedErrno{0};
  std::thread signalThread([&]
  {
    errno = EBUSY;
    for (int i = 0; i < 200000; ++i)
    {
      raise(SIGINT);
      signalCount.store(i + 1);
      if (stopRequested.load())
        break;
    }
    observedErrno.store(errno);
    finished.store(true);
  });

  int lastCount = 0;
  int stalledChecks = 0;
  while (!finished.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const int currentCount = signalCount.load();
    if (currentCount == lastCount)
    {
      ++stalledChecks;
      if (stalledChecks >= 10)
      {
        stopRequested.store(true);
        pthread_kill(signalThread.native_handle(), SIGUSR1);
        break;
      }
    }
    else
    {
      stalledChecks = 0;
      lastCount = currentCount;
    }
  }

  for (int i = 0; i < 50 && !finished.load(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

  if (finished.load())
  {
    signalThread.join();
    _exit(observedErrno.load() == EBUSY ? 0 : 1);
  }

  _exit(1);
}

//////////////////////////////////////////////////
void VerifyWaitForShutdownWakesAllWaiters()
{
  std::atomic<int> readyCount{0};
  std::atomic<int> completedCount{0};

  auto waiter = [&]
  {
    ++readyCount;
    transport::waitForShutdown();
    ++completedCount;
  };

  std::thread firstThread(waiter);
  std::thread secondThread(waiter);
  (void)firstThread;
  (void)secondThread;

  for (int i = 0; i < 50 && readyCount.load() < 2; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  for (int i = 0; i < 50 && completedCount.load() < 2; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  if (completedCount.load() != 2)
    _exit(1);

  firstThread.join();
  secondThread.join();
  _exit(0);
}
}  // namespace
#endif

//////////////////////////////////////////////////
/// \brief waitUntil: predicate immediately returns true near-instantly.
TEST(WaitHelpersTest, WaitUntilImmediatelyTrue)
{
  auto t1 = std::chrono::steady_clock::now();
  EXPECT_TRUE(transport::waitUntil([]{ return true; }));
  auto t2 = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  EXPECT_LT(elapsed, 100);
}

//////////////////////////////////////////////////
/// \brief waitUntil: predicate never returns false after timeout.
TEST(WaitHelpersTest, WaitUntilNeverTrue)
{
  auto t1 = std::chrono::steady_clock::now();
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(100)));
  auto t2 = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  EXPECT_GE(elapsed, 90);
  EXPECT_LT(elapsed, 500);
}

//////////////////////////////////////////////////
/// \brief waitUntil: predicate becomes true mid-wait.
TEST(WaitHelpersTest, WaitUntilBecomesTrueMidWait)
{
  std::atomic<int> callCount{0};
  auto pred = [&]() -> bool {
    return ++callCount >= 5;
  };
  EXPECT_TRUE(transport::waitUntil(pred, std::chrono::milliseconds(5000),
      std::chrono::milliseconds(10)));
  EXPECT_GE(callCount.load(), 5);
}

//////////////////////////////////////////////////
/// \brief waitUntil: zero timeout.
TEST(WaitHelpersTest, WaitUntilZeroTimeout)
{
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(0)));
  EXPECT_TRUE(transport::waitUntil([]{ return true; },
      std::chrono::milliseconds(0)));
}

//////////////////////////////////////////////////
/// \brief waitUntil: elapsed time is close to timeout on failure.
TEST(WaitHelpersTest, WaitUntilElapsedCloseToTimeout)
{
  auto t1 = std::chrono::steady_clock::now();
  EXPECT_FALSE(transport::waitUntil([]{ return false; },
      std::chrono::milliseconds(200)));
  auto t2 = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  EXPECT_GE(elapsed, 180);
  EXPECT_LT(elapsed, 1000);
}

//////////////////////////////////////////////////
/// \brief waitForService: service not advertised returns false.
TEST(WaitHelpersTest, WaitForServiceNotAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;
  EXPECT_FALSE(transport::waitForService(node, "/nonexistent",
      std::chrono::milliseconds(200)));
}

//////////////////////////////////////////////////
/// \brief waitForService: service already advertised returns true.
TEST(WaitHelpersTest, WaitForServiceAlreadyAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;

  std::function<bool(const msgs::Int32 &, msgs::Int32 &)> cb =
      [](const msgs::Int32 &_req, msgs::Int32 &_rep) -> bool {
    _rep.set_data(_req.data());
    return true;
  };
  EXPECT_TRUE(node.Advertise("/test_svc", cb));

  EXPECT_TRUE(transport::waitForService(node, "/test_svc",
      std::chrono::milliseconds(2000)));
}

//////////////////////////////////////////////////
/// \brief waitForTopic: topic already advertised returns true.
TEST(WaitHelpersTest, WaitForTopicAlreadyAdvertised)
{
  gz::utils::setenv("GZ_PARTITION", testing::getRandomNumber());
  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/test_topic");
  EXPECT_TRUE(pub);

  EXPECT_TRUE(transport::waitForTopic(node, "/test_topic",
      std::chrono::milliseconds(2000)));
}

//////////////////////////////////////////////////
/// \brief Create a separate thread, block it by calling waitForShutdown() and
/// emit a SIGINT signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(WaitHelpersTest, waitForShutdownSIGINT)
{
  std::thread aThread([]{transport::waitForShutdown();});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGINT);
  aThread.join();
}

//////////////////////////////////////////////////
/// \brief Create a separate thread, block it by calling waitForShutdown() and
/// emit a SIGTERM signal. Check that the transport library captures the signal
/// and is able to terminate.
TEST(WaitHelpersTest, waitForShutdownSIGTERM)
{
  std::thread aThread([]{transport::waitForShutdown();});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  raise(SIGTERM);
  aThread.join();
}

//////////////////////////////////////////////////
/// \brief Stress-test re-entry into waitForShutdown(). The implementation
/// keeps a process-wide self-pipe alive across calls; this test loops the
/// wait-and-signal cycle to flush out edge cases in the persistent state
/// (e.g. left-over bytes in the pipe, handler installation order).
TEST(WaitHelpersTest, waitForShutdownReEntryStress)
{
  for (int i = 0; i < 10; ++i)
  {
    std::thread aThread([]{transport::waitForShutdown();});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    raise(SIGINT);
    aThread.join();
  }
}

#ifndef _WIN32
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-default"
#endif
//////////////////////////////////////////////////
/// \brief One shutdown signal should release every blocked waiter. The current
/// implementation only wakes one waiter, so this test fails in a plain build
/// until the process-wide wake semantics are restored.
TEST(WaitHelpersTest, waitForShutdownSingleSignalOnlyWakesOneWaiter)
{
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  ASSERT_EXIT(VerifyWaitForShutdownWakesAllWaiters(),
      ::testing::ExitedWithCode(0), "");
}

//////////////////////////////////////////////////
/// \brief Repeated shutdown signals should not block once there is no waiter
/// draining the persistent self-pipe. The current blocking pipe makes the
/// signal path stall, so this test fails in a plain build until the pipe is
/// made nonblocking.
TEST(WaitHelpersTest, waitForShutdownBurstSignalsCanBlockSignalHandler)
{
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  ASSERT_EXIT(VerifyWaitForShutdownBurstSignalsStayNonBlocking(),
      ::testing::ExitedWithCode(0), "");
}

//////////////////////////////////////////////////
/// \brief A shutdown signal should preserve the interrupted thread's errno.
/// The current handler leaks the write() failure errno back to the caller, so
/// this test fails in a plain build until errno is saved and restored.
TEST(WaitHelpersTest, waitForShutdownSignalHandlerCanClobberErrno)
{
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  ASSERT_EXIT(VerifyWaitForShutdownPreservesErrno(),
      ::testing::ExitedWithCode(0), "");
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif
