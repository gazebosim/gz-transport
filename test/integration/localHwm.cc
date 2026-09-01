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
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_utils.hh"

using namespace gz;
using namespace std::chrono_literals;

// The capacity of the local publication queue used in this test. It is set
// via the GZ_TRANSPORT_LOCAL_HWM environment variable in main().
static const int kLocalHwm = 5;

/// \brief Helper that subscribes to a topic and optionally blocks the local
/// delivery thread inside the callback of the first received message until
/// the test releases it. All the received values are recorded.
class BlockingSubscriber
{
  /// \brief Constructor.
  /// \param[in] _node Node used to subscribe.
  /// \param[in] _topic Topic name.
  /// \param[in] _blockFirst True to block the delivery thread inside the
  /// callback of the first received message.
  public: BlockingSubscriber(transport::Node &_node,
    const std::string &_topic, bool _blockFirst = true)
  {
    this->cb = [this, _blockFirst](const msgs::Int32 &_msg)
      {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->received.push_back(_msg.data());
        this->cv.notify_all();
        if (_blockFirst && this->received.size() == 1u)
        {
          // Block the local delivery thread until ReleaseAndWait() is
          // called.
          this->cv.wait_for(lk, 30s, [this]{return this->released;});
        }
      };
    EXPECT_TRUE(_node.Subscribe(_topic, this->cb));
  }

  /// \brief Wait until the delivery thread is blocked inside the callback
  /// of the first received message.
  /// \return True if the first message was received before the timeout.
  public: bool WaitFirstMsg()
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    return this->cv.wait_for(lk, 10s, [this]{return !this->received.empty();});
  }

  /// \brief Wait until _count messages have been received in total.
  /// \param[in] _count Expected total number of received messages.
  /// \return True if _count messages were received before the timeout.
  public: bool WaitReceived(size_t _count)
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    return this->cv.wait_for(lk, 10s,
      [this, _count]{return this->received.size() >= _count;});
  }

  /// \brief Unblock the delivery thread and wait until _count messages have
  /// been received in total.
  /// \param[in] _count Expected total number of received messages.
  /// \return True if _count messages were received before the timeout.
  public: bool ReleaseAndWait(size_t _count)
  {
    {
      std::lock_guard<std::mutex> lk(this->mutex);
      this->released = true;
    }
    this->cv.notify_all();
    return this->WaitReceived(_count);
  }

  /// \brief Get the values received so far.
  /// \return The received values, in reception order.
  public: std::vector<int> Received()
  {
    std::lock_guard<std::mutex> lk(this->mutex);
    return this->received;
  }

  /// \brief Protects all the members below.
  private: std::mutex mutex;

  /// \brief Signals new received messages and the release of the callback.
  private: std::condition_variable cv;

  /// \brief True when the callback should stop blocking.
  private: bool released = false;

  /// \brief Values received so far.
  private: std::vector<int> received;

  /// \brief Subscription callback.
  private: std::function<void(const msgs::Int32 &)> cb;
};

//////////////////////////////////////////////////
/// \brief Check that when a topic exceeds the capacity of the local
/// publication queue, its oldest queued messages are dropped and the
/// newest ones are delivered.
TEST(localHwmTest, DropOldestWhenFull)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/foo");
  ASSERT_TRUE(pub);

  BlockingSubscriber sub(node, "/foo");

  // Publish a first message and wait until the delivery thread is blocked
  // inside its callback. At this point the local publication queue is empty.
  msgs::Int32 msg;
  msg.set_data(0);
  EXPECT_TRUE(pub.Publish(msg));
  ASSERT_TRUE(sub.WaitFirstMsg());

  // Publish more messages than the capacity of the queue. The oldest
  // messages should be dropped, keeping only the newest kLocalHwm ones.
  for (int i = 1; i <= 2 * kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pub.Publish(msg));
  }

  // Unblock the delivery thread and wait for the queued messages.
  ASSERT_TRUE(sub.ReleaseAndWait(1u + kLocalHwm));

  // Give the delivery thread some time to process unexpected extra messages.
  std::this_thread::sleep_for(200ms);

  // We expect the first message and the newest kLocalHwm ones.
  std::vector<int> expected = {0, 6, 7, 8, 9, 10};
  EXPECT_EQ(expected, sub.Received());
}

//////////////////////////////////////////////////
/// \brief Check that no messages are dropped while a topic stays within the
/// capacity of the local publication queue.
TEST(localHwmTest, NoDropsUnderCapacity)
{
  ASSERT_EQ(kLocalHwm, transport::localHwm());

  transport::Node node;
  auto pub = node.Advertise<msgs::Int32>("/bar");
  ASSERT_TRUE(pub);

  BlockingSubscriber sub(node, "/bar");

  // Publish a first message and wait until the delivery thread is blocked
  // inside its callback. At this point the local publication queue is empty.
  msgs::Int32 msg;
  msg.set_data(100);
  EXPECT_TRUE(pub.Publish(msg));
  ASSERT_TRUE(sub.WaitFirstMsg());

  // Fill the queue up to its capacity. No messages should be dropped.
  for (int i = 101; i <= 100 + kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pub.Publish(msg));
  }

  // Unblock the delivery thread and wait for the queued messages.
  ASSERT_TRUE(sub.ReleaseAndWait(1u + kLocalHwm));

  std::vector<int> expected = {100, 101, 102, 103, 104, 105};
  EXPECT_EQ(expected, sub.Received());
}

//////////////////////////////////////////////////
/// \brief Check that a topic exceeding its queue capacity does not cause
/// drops on other topics, even when their queued messages are older.
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

  // Publish a first message and wait until the delivery thread is blocked
  // inside its callback.
  msgs::Int32 msg;
  msg.set_data(0);
  EXPECT_TRUE(pubA.Publish(msg));
  ASSERT_TRUE(subA.WaitFirstMsg());

  // Queue a few messages of the quiet topic first, so that they become the
  // oldest entries of the whole queue.
  for (int i = 1; i <= 3; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pubB.Publish(msg));
  }

  // Flood the noisy topic beyond its capacity. Only its own oldest
  // messages should be dropped, never the older messages of the quiet
  // topic.
  for (int i = 1; i <= 2 * kLocalHwm; ++i)
  {
    msg.set_data(i);
    EXPECT_TRUE(pubA.Publish(msg));
  }

  // Unblock the delivery thread and wait for all the expected messages.
  ASSERT_TRUE(subA.ReleaseAndWait(1u + kLocalHwm));
  ASSERT_TRUE(subB.WaitReceived(3u));

  std::vector<int> expectedA = {0, 6, 7, 8, 9, 10};
  std::vector<int> expectedB = {1, 2, 3};
  EXPECT_EQ(expectedA, subA.Received());
  EXPECT_EQ(expectedB, subB.Received());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  std::string partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);

  // Use a small local publication queue to make it easy to fill it up.
  gz::utils::setenv("GZ_TRANSPORT_LOCAL_HWM", std::to_string(kLocalHwm));

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
