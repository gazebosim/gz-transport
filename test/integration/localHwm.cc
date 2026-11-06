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
#include <gz/msgs/int32.pb.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_utils.hh"

using namespace gz;
using namespace std::chrono_literals;

// Local queue capacity, set via GZ_TRANSPORT_LOCAL_HWM in main().
static const int kLocalHwm = 5;

/// \brief Records received values and can block the delivery thread on
/// the first message until released.
class BlockingSubscriber
{
  /// \brief Constructor.
  /// \param[in] _node Node used to subscribe.
  /// \param[in] _topic Topic name.
  /// \param[in] _blockFirst True to block on the first message.
  public: BlockingSubscriber(transport::Node &_node,
    const std::string &_topic, bool _blockFirst = true)
  {
    std::function<void(const msgs::Int32 &)> cb =
      [this, _blockFirst](const msgs::Int32 &_msg)
      {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->received.push_back(_msg.data());
        this->cv.notify_all();
        if (_blockFirst && this->received.size() == 1u)
        {
          // Block the delivery thread until ReleaseAndWait().
          this->cv.wait_for(lk, 30s, [this]{return this->released;});
        }
      };
    this->sub = _node.CreateSubscriber(_topic, cb);
    EXPECT_TRUE(this->sub);
  }

  /// \brief Wait for the first message.
  /// \return True if received before the timeout.
  public: bool WaitFirstMsg()
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    return this->cv.wait_for(lk, 10s, [this]{return !this->received.empty();});
  }

  /// \brief Wait until _count messages were received in total.
  /// \param[in] _count Expected total.
  /// \return True if reached before the timeout.
  public: bool WaitReceived(size_t _count)
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    return this->cv.wait_for(lk, 10s,
      [this, _count]{return this->received.size() >= _count;});
  }

  /// \brief Unblock the delivery thread and call WaitReceived(_count).
  /// \param[in] _count Expected total.
  /// \return True if reached before the timeout.
  public: bool ReleaseAndWait(size_t _count)
  {
    {
      std::lock_guard<std::mutex> lk(this->mutex);
      this->released = true;
    }
    this->cv.notify_all();
    return this->WaitReceived(_count);
  }

  /// \brief Get the values received so far, in order.
  /// \return The received values.
  public: std::vector<int> Received()
  {
    std::lock_guard<std::mutex> lk(this->mutex);
    return this->received;
  }

  /// \brief Protects all the members below.
  private: std::mutex mutex;

  /// \brief Signals new messages and the release.
  private: std::condition_variable cv;

  /// \brief True when the callback should stop blocking.
  private: bool released = false;

  /// \brief Values received so far.
  private: std::vector<int> received;

  /// \brief Declared last so it unsubscribes first.
  private: transport::Node::Subscriber sub;
};

/// \brief Published last; delivery is FIFO, so nothing is pending after it.
static const int kSentinel = -1;

//////////////////////////////////////////////////
/// \brief A full topic drops its oldest messages and keeps the newest.
TEST(localHwmTest, DropOldestWhenFull)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/foo");
  ASSERT_TRUE(pub);

  BlockingSubscriber sub(node, "/foo");

  // Block the delivery thread on the first message.
  msgs::Int32 msg;
  msg.set_data(0);
  EXPECT_TRUE(pub.Publish(msg));
  ASSERT_TRUE(sub.WaitFirstMsg());

  // Overflow the queue: only the newest kLocalHwm should remain.
  for (int i = 1; i <= 2 * kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pub.Publish(msg));
  }

  ASSERT_TRUE(sub.ReleaseAndWait(1u + kLocalHwm));

  // Any unexpected extra message would arrive before the sentinel.
  msg.set_data(kSentinel);
  EXPECT_TRUE(pub.Publish(msg));
  ASSERT_TRUE(sub.WaitReceived(2u + kLocalHwm));

  std::vector<int> expected = {0, 6, 7, 8, 9, 10, kSentinel};
  EXPECT_EQ(expected, sub.Received());
}

//////////////////////////////////////////////////
/// \brief No drops while a topic stays within capacity.
TEST(localHwmTest, NoDropsUnderCapacity)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/bar");
  ASSERT_TRUE(pub);

  BlockingSubscriber sub(node, "/bar");

  // Block the delivery thread on the first message.
  msgs::Int32 msg;
  msg.set_data(100);
  EXPECT_TRUE(pub.Publish(msg));
  ASSERT_TRUE(sub.WaitFirstMsg());

  // Fill the queue exactly to capacity.
  for (int i = 101; i <= 100 + kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pub.Publish(msg));
  }

  ASSERT_TRUE(sub.ReleaseAndWait(1u + kLocalHwm));

  std::vector<int> expected = {100, 101, 102, 103, 104, 105};
  EXPECT_EQ(expected, sub.Received());
}

//////////////////////////////////////////////////
/// \brief A full topic never drops other topics' messages, even older ones.
TEST(localHwmTest, TopicIsolation)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  transport::Node node;
  auto pubA = node.Advertise<msgs::Int32>("/iso_a");
  auto pubB = node.Advertise<msgs::Int32>("/iso_b");
  ASSERT_TRUE(pubA);
  ASSERT_TRUE(pubB);

  BlockingSubscriber subA(node, "/iso_a");
  BlockingSubscriber subB(node, "/iso_b", false);

  // Block the delivery thread on the first message.
  msgs::Int32 msg;
  msg.set_data(0);
  EXPECT_TRUE(pubA.Publish(msg));
  ASSERT_TRUE(subA.WaitFirstMsg());

  // Queue the quiet topic first, so its messages are the oldest.
  for (int i = 1; i <= 3; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pubB.Publish(msg));
  }

  // Overflow the noisy topic.
  for (int i = 1; i <= 2 * kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pubA.Publish(msg));
  }

  ASSERT_TRUE(subA.ReleaseAndWait(1u + kLocalHwm));
  ASSERT_TRUE(subB.WaitReceived(3u));

  std::vector<int> expectedA = {0, 6, 7, 8, 9, 10};
  std::vector<int> expectedB = {1, 2, 3};
  EXPECT_EQ(expectedA, subA.Received());
  EXPECT_EQ(expectedB, subB.Received());
}

//////////////////////////////////////////////////
/// \brief The capacity also applies to raw subscribers.
TEST(localHwmTest, RawSubscriber)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  // Declared before the node to outlive the subscription.
  std::mutex mutex;
  std::vector<int> received;

  transport::Node node;
  auto pubGate = node.Advertise<msgs::Int32>("/raw_gate");
  auto pub = node.Advertise<msgs::Int32>("/raw");
  ASSERT_TRUE(pubGate);
  ASSERT_TRUE(pub);

  EXPECT_TRUE(node.SubscribeRaw("/raw",
    [&](const char *_data, size_t _size, const transport::MessageInfo &)
    {
      msgs::Int32 rawMsg;
      ASSERT_TRUE(rawMsg.ParseFromArray(_data, static_cast<int>(_size)));
      std::lock_guard<std::mutex> lk(mutex);
      received.push_back(rawMsg.data());
    },
    msgs::Int32().GetTypeName()));

  // Block the delivery thread with a message on another topic.
  BlockingSubscriber gate(node, "/raw_gate");
  msgs::Int32 msg;
  msg.set_data(0);
  EXPECT_TRUE(pubGate.Publish(msg));
  ASSERT_TRUE(gate.WaitFirstMsg());

  for (int i = 1; i <= 2 * kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pub.Publish(msg));
  }

  ASSERT_TRUE(gate.ReleaseAndWait(1u));
  ASSERT_TRUE(transport::waitUntil([&]
  {
    std::lock_guard<std::mutex> lk(mutex);
    return received.size() >= static_cast<size_t>(kLocalHwm);
  }));

  std::lock_guard<std::mutex> lk(mutex);
  std::vector<int> expected = {6, 7, 8, 9, 10};
  EXPECT_EQ(expected, received);
}

//////////////////////////////////////////////////
/// \brief Count the drop warnings printed for a topic.
/// \param[in] _output Captured std::cerr.
/// \param[in] _topic Topic name.
/// \return Number of drop warnings for _topic.
static int CountDropWarnings(const std::string &_output,
  const std::string &_topic)
{
  const std::string pattern = _topic + "] are processing messages slower";
  int count = 0;
  for (auto pos = _output.find(pattern); pos != std::string::npos;
       pos = _output.find(pattern, pos + pattern.size()))
  {
    ++count;
  }
  return count;
}

//////////////////////////////////////////////////
/// \brief The drop warning prints once per overload, and again after the
/// topic drains.
TEST(localHwmTest, WarnOncePerEpisode)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  transport::Node node;
  auto pubGate1 = node.Advertise<msgs::Int32>("/warn_gate1");
  auto pubGate2 = node.Advertise<msgs::Int32>("/warn_gate2");
  auto pub = node.Advertise<msgs::Int32>("/warn");
  ASSERT_TRUE(pubGate1);
  ASSERT_TRUE(pubGate2);
  ASSERT_TRUE(pub);

  BlockingSubscriber gate1(node, "/warn_gate1");
  BlockingSubscriber gate2(node, "/warn_gate2");
  BlockingSubscriber sub(node, "/warn", false);

  // The warning is printed by Publish(), so capture std::cerr around it.
  std::ostringstream captured;
  auto flood = [&](int _first)
  {
    auto *original = std::cerr.rdbuf(captured.rdbuf());
    msgs::Int32 msg;
    for (int i = _first; i < _first + 3 * kLocalHwm; ++i)
    {
      msg.set_data(i);
      EXPECT_TRUE(pub.Publish(msg));
    }
    std::cerr.rdbuf(original);
  };

  // First overload: one warning.
  msgs::Int32 msg;
  EXPECT_TRUE(pubGate1.Publish(msg));
  ASSERT_TRUE(gate1.WaitFirstMsg());
  flood(0);
  EXPECT_EQ(1, CountDropWarnings(captured.str(), "/warn"));

  // Drain the topic.
  ASSERT_TRUE(gate1.ReleaseAndWait(1u));
  ASSERT_TRUE(sub.WaitReceived(kLocalHwm));

  // Second overload: warns again.
  EXPECT_TRUE(pubGate2.Publish(msg));
  ASSERT_TRUE(gate2.WaitFirstMsg());
  flood(100);
  EXPECT_EQ(2, CountDropWarnings(captured.str(), "/warn"));

  ASSERT_TRUE(gate2.ReleaseAndWait(1u));
  ASSERT_TRUE(sub.WaitReceived(2u * kLocalHwm));
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  std::string partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);

  // A small queue is easy to fill.
  gz::utils::setenv("GZ_TRANSPORT_LOCAL_HWM", std::to_string(kLocalHwm));

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
