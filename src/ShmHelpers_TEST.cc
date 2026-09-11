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

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

#include <gz/msgs/int32.pb.h>

#include "gz/transport/config.hh"
#include "gz/transport/SubscriptionHandler.hh"

// Self-guarding: brings in zenoh.hxx (and the Z_FEATURE_* macros) only
// when Zenoh is available.
#include "ShmHelpers.hh"

using namespace gz;
using namespace transport;

//////////////////////////////////////////////////
// CreateMsgFromBuffer: typed SubscriptionHandler parses from a raw buffer
// through the message factory registered on construction. Requires
// neither Zenoh nor SHM.
TEST(ShmHelpersTest, CreateMsgFromBufferTyped)
{
  SubscribeOptions opts;
  SubscriptionHandler<gz::msgs::Int32> handler(
    "proc-uuid", "node-uuid", opts);

  // Serialize a message.
  gz::msgs::Int32 original;
  original.set_data(42);
  std::string serialized;
  ASSERT_TRUE(original.SerializeToString(&serialized));

  // Deserialize from raw buffer.
  auto msg = handler.CreateMsgFromBuffer(
    serialized.data(), serialized.size(), "gz.msgs.Int32");
  ASSERT_NE(msg, nullptr);

  auto *typed = dynamic_cast<const gz::msgs::Int32 *>(msg.get());
  ASSERT_NE(typed, nullptr);
  EXPECT_EQ(42, typed->data());
}

//////////////////////////////////////////////////
// CreateMsgFromBuffer: invalid data returns non-null but
// may have default values (ParseFromArray on garbage).
TEST(ShmHelpersTest, CreateMsgFromBufferInvalidData)
{
  SubscribeOptions opts;
  SubscriptionHandler<gz::msgs::Int32> handler(
    "proc-uuid", "node-uuid", opts);

  // Pass garbage data — ParseFromArray may succeed with default values
  // or fail. Either way, it should not crash.
  const char garbage[] = {0x00, 0x01, 0x02, 0x03};
  auto msg = handler.CreateMsgFromBuffer(
    garbage, sizeof(garbage), "gz.msgs.Int32");
  // Just verify no crash — result may or may not be null.
  (void)msg;
}

//////////////////////////////////////////////////
// CreateMsgFromBuffer: generic SubscriptionHandler<ProtoMsg>
TEST(ShmHelpersTest, CreateMsgFromBufferGeneric)
{
  SubscribeOptions opts;
  SubscriptionHandler<ProtoMsg> handler(
    "proc-uuid", "node-uuid", opts);

  // Serialize a message.
  gz::msgs::Int32 original;
  original.set_data(99);
  std::string serialized;
  ASSERT_TRUE(original.SerializeToString(&serialized));

  // Deserialize from raw buffer using the generic handler,
  // which looks up the type by name at runtime.
  auto msg = handler.CreateMsgFromBuffer(
    serialized.data(), serialized.size(), "gz.msgs.Int32");
  ASSERT_NE(msg, nullptr);

  auto *typed = dynamic_cast<const gz::msgs::Int32 *>(msg.get());
  ASSERT_NE(typed, nullptr);
  EXPECT_EQ(99, typed->data());
}

//////////////////////////////////////////////////
// CreateMsgFromBuffer: generic handler with unknown type
TEST(ShmHelpersTest, CreateMsgFromBufferGenericUnknownType)
{
  SubscribeOptions opts;
  SubscriptionHandler<ProtoMsg> handler(
    "proc-uuid", "node-uuid", opts);

  const char data[] = {0x08, 0x01};
  auto msg = handler.CreateMsgFromBuffer(
    data, sizeof(data), "gz.msgs.NonExistentType");
  EXPECT_EQ(nullptr, msg);
}

#ifdef HAVE_ZENOH

//////////////////////////////////////////////////
// withPayloadView / payloadToString: see the payload of heap-backed
// bytes. Requires Zenoh but not SHM.
TEST(ShmHelpersTest, WithPayloadViewHeap)
{
  const std::string data = "hello payload";
  zenoh::Bytes bytes(data);

  auto copied = withPayloadView(bytes,
    [](const char *_data, std::size_t _size)
    {
      return std::string(_data, _size);
    });
  EXPECT_EQ(data, copied);
  EXPECT_EQ(data, payloadToString(bytes));
}

// The remaining tests exercise the real SHM helpers, which need both the
// SHM feature and the unstable API (same guard as ShmHelpers.hh).
#if defined(Z_FEATURE_SHARED_MEMORY) && defined(Z_FEATURE_UNSTABLE_API)

/// \brief Threshold used by the SHM tests.
static constexpr std::size_t kTestThreshold = 4096;

/// \brief Pool size used by the SHM tests.
static constexpr std::size_t kTestPoolSize = 4u * 1024u * 1024u;

//////////////////////////////////////////////////
/// \brief Open a Zenoh session with the SHM subsystem initialized eagerly
/// so the provider is available right after open.
/// \param[in] _shmEnabled Whether to enable shared memory at all.
/// \return The session, or nullptr if it could not be opened (e.g. the
/// host cannot provide shared memory).
static std::shared_ptr<zenoh::Session> OpenTestSession(bool _shmEnabled)
{
  zenoh::Config config = zenoh::Config::create_default();
  config.insert_json5("transport/shared_memory/enabled",
      _shmEnabled ? "true" : "false");
  config.insert_json5("transport/shared_memory/mode", "\"init\"");
  config.insert_json5(kZenohShmPoolSizeKey, std::to_string(kTestPoolSize));
  config.insert_json5(kZenohShmThresholdKey,
      std::to_string(kTestThreshold));

  try
  {
    return std::make_shared<zenoh::Session>(
      zenoh::Session::open(std::move(config)));
  }
  catch (const zenoh::ZException &)
  {
    return nullptr;
  }
}

//////////////////////////////////////////////////
/// \brief Wait until the session's provider is ready.
/// \return The provider, or nullptr after the timeout.
static const zenoh::ShmProvider *WaitForProvider()
{
  const auto deadline =
    std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (std::chrono::steady_clock::now() < deadline)
  {
    if (auto *provider = ZenohShm::Instance().Provider())
      return provider;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return nullptr;
}

//////////////////////////////////////////////////
/// \brief Fixture attaching a SHM-enabled session to the process-wide
/// state and detaching it afterwards.
class ZenohShmTest : public ::testing::Test
{
  protected: void SetUp() override
  {
    this->session = OpenTestSession(true);
    if (!this->session)
      GTEST_SKIP() << "Shared memory unavailable in this environment";

    initZenohShm(this->session, kTestThreshold);
    this->provider = WaitForProvider();
    if (!this->provider)
      GTEST_SKIP() << "Session SHM provider never became ready";
  }

  protected: void TearDown() override
  {
    // Drop the provider handle before the session goes away.
    initZenohShm(nullptr, kDefaultZenohShmThreshold);
    this->session.reset();
  }

  protected: std::shared_ptr<zenoh::Session> session;
  protected: const zenoh::ShmProvider *provider{nullptr};
};

//////////////////////////////////////////////////
// Without a session everything falls back to the heap path.
TEST(ShmHelpersTest, NoSession)
{
  initZenohShm(nullptr, 100);
  auto &shm = ZenohShm::Instance();
  EXPECT_EQ(100u, shm.Threshold());
  EXPECT_EQ(nullptr, shm.Provider());
  EXPECT_FALSE(static_cast<bool>(allocShmChunk(1000)));
  EXPECT_FALSE(makeShmBytes("x", 1000).has_value());
  EXPECT_FALSE(allocShmBuf(nullptr, 1000).has_value());
}

//////////////////////////////////////////////////
// SHM disabled in the session configuration: the provider stays null.
TEST(ShmHelpersTest, DisabledInConfig)
{
  auto session = OpenTestSession(false);
  if (!session)
    GTEST_SKIP() << "Unable to open a Zenoh session";

  initZenohShm(session, kTestThreshold);
  auto &shm = ZenohShm::Instance();
  EXPECT_EQ(nullptr, shm.Provider());
  // A second call must not flip the answer.
  EXPECT_EQ(nullptr, shm.Provider());
  EXPECT_FALSE(static_cast<bool>(allocShmChunk(kTestThreshold)));

  initZenohShm(nullptr, kDefaultZenohShmThreshold);
}

//////////////////////////////////////////////////
// The provider is the session's and stable across calls.
TEST_F(ZenohShmTest, ProviderIsStable)
{
  EXPECT_EQ(kTestThreshold, ZenohShm::Instance().Threshold());
  EXPECT_EQ(this->provider, ZenohShm::Instance().Provider());
}

//////////////////////////////////////////////////
// allocShmBuf: pool exhaustion fails instead of blocking.
TEST_F(ZenohShmTest, AllocShmBufPoolExhausted)
{
  auto result = allocShmBuf(this->provider, kTestPoolSize + 1);
  EXPECT_FALSE(result.has_value());
}

//////////////////////////////////////////////////
// allocShmBuf: buffers are distinct and writable.
TEST_F(ZenohShmTest, AllocShmBufMultiple)
{
  auto buf1 = allocShmBuf(this->provider, kTestThreshold);
  auto buf2 = allocShmBuf(this->provider, kTestThreshold);
  ASSERT_TRUE(buf1.has_value());
  ASSERT_TRUE(buf2.has_value());
  EXPECT_GE(buf1->len(), kTestThreshold);
  EXPECT_NE(buf1->data(), buf2->data());

  memset(buf1->data(), 0x42, kTestThreshold);
  EXPECT_EQ(0x42, buf1->data()[0]);
  EXPECT_EQ(0x42, buf1->data()[kTestThreshold - 1]);
}

//////////////////////////////////////////////////
// allocShmChunk: empty below threshold, usable at or above it.
TEST_F(ZenohShmTest, AllocShmChunk)
{
  auto emptyChunk = allocShmChunk(kTestThreshold - 1);
  EXPECT_FALSE(static_cast<bool>(emptyChunk));
  EXPECT_EQ(nullptr, emptyChunk.Data());

  auto chunk = allocShmChunk(kTestThreshold);
  ASSERT_TRUE(static_cast<bool>(chunk));
  ASSERT_NE(nullptr, chunk.Data());

  // Write through Data(), then convert to Bytes and read it back.
  memset(chunk.Data(), 0x5A, kTestThreshold);
  zenoh::Bytes bytes = chunk.TakeBytes();
  EXPECT_FALSE(static_cast<bool>(chunk));
  EXPECT_EQ(kTestThreshold, bytes.size());
  EXPECT_EQ(std::string(kTestThreshold, 0x5A), bytes.as_string());
}

//////////////////////////////////////////////////
// makeShmBytes: nullopt below threshold, round-trips data above it.
TEST_F(ZenohShmTest, MakeShmBytes)
{
  EXPECT_FALSE(makeShmBytes("x", 1).has_value());

  const std::string data(kTestThreshold, 'B');
  auto bytes = makeShmBytes(data.data(), data.size());
  ASSERT_TRUE(bytes.has_value());
  EXPECT_EQ(data, bytes->as_string());
}

//////////////////////////////////////////////////
// withPayloadView / payloadToString: see the payload of SHM-backed bytes.
TEST_F(ZenohShmTest, WithPayloadViewShm)
{
  const std::string data(kTestThreshold, 'D');
  auto bytes = makeShmBytes(data.data(), data.size());
  ASSERT_TRUE(bytes.has_value());

  auto copied = withPayloadView(*bytes,
    [](const char *_data, std::size_t _size)
    {
      return std::string(_data, _size);
    });
  EXPECT_EQ(data, copied);
  EXPECT_EQ(data, payloadToString(*bytes));
}

#endif  // Z_FEATURE_SHARED_MEMORY && Z_FEATURE_UNSTABLE_API
#endif  // HAVE_ZENOH

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
