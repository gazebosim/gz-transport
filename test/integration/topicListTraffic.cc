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

#include <gz/msgs/discovery.pb.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "gz/transport/Node.hh"
#include "gz/transport/NodeShared.hh"

#include <gz/utils/Environment.hh>
#include <gz/utils/Subprocess.hh>

#include "gtest/gtest.h"
#include "test_config.hh"
#include "test_utils.hh"

using namespace gz;

static std::string partition;  // NOLINT(*)

// Private discovery port so that this test observes only its own traffic.
static const int kTestDiscPort = 11417;

// Wire version of the discovery protocol. It must match
// Discovery::wireVersion or the crafted messages are discarded.
static const uint32_t kWireVersion = 10;

//////////////////////////////////////////////////
/// \brief Helper joining the discovery multicast group with a raw UDP
/// socket. It can passively count discovery messages by type and send
/// crafted discovery messages, emulating a remote process.
class DiscoveryWire
{
  public: DiscoveryWire()
  {
    this->sock = socket(AF_INET, SOCK_DGRAM, 0);
    EXPECT_GE(this->sock, 0);

    int reuse = 1;
    setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(this->sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(kTestDiscPort);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    EXPECT_EQ(bind(this->sock,
      reinterpret_cast<sockaddr *>(&local), sizeof(local)), 0);

    ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.0.7");
    mreq.imr_interface.s_addr = inet_addr("127.0.0.1");
    EXPECT_EQ(setsockopt(this->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
      &mreq, sizeof(mreq)), 0);

    in_addr iface{};
    iface.s_addr = inet_addr("127.0.0.1");
    setsockopt(this->sock, IPPROTO_IP, IP_MULTICAST_IF,
      &iface, sizeof(iface));
    int loop = 1;
    setsockopt(this->sock, IPPROTO_IP, IP_MULTICAST_LOOP,
      &loop, sizeof(loop));

    fcntl(this->sock, F_SETFL, O_NONBLOCK);

    this->dst = {};
    this->dst.sin_family = AF_INET;
    this->dst.sin_port = htons(kTestDiscPort);
    this->dst.sin_addr.s_addr = inet_addr("239.255.0.7");
  }

  public: ~DiscoveryWire()
  {
    close(this->sock);
  }

  /// \brief Send a discovery message to the multicast group, framed with
  /// the 2 byte length prefix used by the discovery wire format.
  public: void Send(const gz::msgs::Discovery &_msg)
  {
    const uint16_t msgSize = static_cast<uint16_t>(_msg.ByteSizeLong());
    std::vector<char> buffer(sizeof(msgSize) + msgSize);
    memcpy(buffer.data(), &msgSize, sizeof(msgSize));
    ASSERT_TRUE(_msg.SerializeToArray(
      buffer.data() + sizeof(msgSize), msgSize));
    sendto(this->sock, buffer.data(), buffer.size(), 0,
      reinterpret_cast<sockaddr *>(&this->dst), sizeof(this->dst));
  }

  /// \brief Drain the pending datagrams, counting the parsed discovery
  /// messages by type.
  /// \param[in] _windowMs Extra time to keep draining (ms.).
  /// \return Map of message type to number of messages observed.
  public: std::map<int, int> CountTypes(const int _windowMs)
  {
    std::map<int, int> counts;
    const auto deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(_windowMs);

    do
    {
      char buffer[65536];
      ssize_t received;
      while ((received = recvfrom(this->sock, buffer, sizeof(buffer), 0,
        nullptr, nullptr)) > 0)
      {
        uint16_t msgSize;
        if (received < static_cast<ssize_t>(sizeof(msgSize)))
          continue;
        memcpy(&msgSize, buffer, sizeof(msgSize));

        gz::msgs::Discovery msg;
        if (msg.ParseFromArray(buffer + sizeof(msgSize), msgSize))
          ++counts[msg.type()];
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);

    return counts;
  }

  /// \brief Drain and discard all the pending datagrams.
  public: void Drain()
  {
    this->CountTypes(0);
  }

  private: int sock = -1;
  private: sockaddr_in dst;
};

//////////////////////////////////////////////////
/// \brief A known process that never answers a SUBSCRIBERS_REQ makes
/// TopicList() wait for the timeout, and the call recovers once the silent
/// process expires.
TEST(topicListTraffic, TimeoutWithSilentPeer)
{
  transport::Node node;

  // Initialize discovery.
  std::vector<std::string> topics;
  node.TopicList(topics);

  // Emulate a remote process that heartbeats but never answers.
  DiscoveryWire wire;
  gz::msgs::Discovery heartbeat;
  heartbeat.set_version(kWireVersion);
  heartbeat.set_type(gz::msgs::Discovery::HEARTBEAT);
  heartbeat.set_process_uuid("topicListTraffic-silent-peer");
  for (int i = 0; i < 3; ++i)
  {
    wire.Send(heartbeat);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // The silent peer is known but never reports its subscribers: the call
  // returns when the timeout expires.
  auto start = std::chrono::steady_clock::now();
  node.TopicList(topics);
  auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start).count();
  EXPECT_GE(elapsedMs, 90);
  EXPECT_LT(elapsedMs, 250);

  // After the silence interval the peer expires and the calls are fast
  // again.
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));
  start = std::chrono::steady_clock::now();
  node.TopicList(topics);
  elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start).count();
  EXPECT_LT(elapsedMs, 50);
}

//////////////////////////////////////////////////
/// \brief The subscribers traffic is proportional to the demand: nothing is
/// requested while idle and a single TopicList() call produces one request
/// and one bounded reply burst.
TEST(topicListTraffic, TrafficBounds)
{
  transport::Node node;

  // A remote process with one subscription.
  auto pi = testing::SubprocessJoinWrapper(
    {test_executables::kSubscriberOnly, partition, "/subscriber_only", "15"});

  // Let the remote process start and its discovery settle.
  std::this_thread::sleep_for(std::chrono::seconds(3));

  DiscoveryWire wire;
  wire.Drain();

  // While idle, no subscribers traffic flows.
  auto counts = wire.CountTypes(2000);
  EXPECT_EQ(counts[gz::msgs::Discovery::SUBSCRIBERS_REQ], 0);
  EXPECT_EQ(counts[gz::msgs::Discovery::SUBSCRIBERS_REP], 0);

  // A single call produces one request and one reply from the remote
  // process, which has a single subscription.
  std::vector<std::string> topics;
  node.TopicList(topics);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  counts = wire.CountTypes(0);
  EXPECT_EQ(counts[gz::msgs::Discovery::SUBSCRIBERS_REQ], 1);
  EXPECT_EQ(counts[gz::msgs::Discovery::SUBSCRIBERS_REP], 1);

  EXPECT_TRUE(std::find(topics.begin(), topics.end(), "/subscriber_only") !=
    topics.end());
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Get a random partition name.
  partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", partition);

  // Use a private discovery port so that this test observes only its own
  // traffic.
  gz::utils::setenv("GZ_DISCOVERY_MSG_PORT", std::to_string(kTestDiscPort));
  gz::utils::setenv("GZ_DISCOVERY_SRV_PORT",
    std::to_string(kTestDiscPort + 1));

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
