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

#include "gz/transport/config.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>

#if defined(Z_FEATURE_SHARED_MEMORY)

#include <string>

#include <gz/msgs/int32.pb.h>
#include <gz/utils/Environment.hh>

#include "gz/transport/Helpers.hh"
#include "gz/transport/Node.hh"
#include "gz/transport/SubscriptionHandler.hh"
#include "ShmHelpers.hh"

using namespace gz;
using namespace transport;

//////////////////////////////////////////////////
// shmEnvConfig: defaults
TEST(ShmHelpersTest, ShmEnvConfigDefaults)
{
  // Note: shmEnvConfig() caches on first call, so these tests verify
  // the compile-time defaults. Environment variable overrides are
  // tested in a separate process by the Node_TEST suite.
  const auto &config = shmEnvConfig();

  // Default: enabled (true unless Zenoh config disables it).
  EXPECT_TRUE(config.enabled);

  // Default pool size: 48 MB.
  EXPECT_EQ(48u * 1024u * 1024u, config.poolSize);

  // Default threshold: 128 KB.
  EXPECT_EQ(128u * 1024u, config.threshold);
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: valid positive value
TEST(ShmHelpersTest, ParseShmSizeEnvVarValid)
{
  auto result = parseShmSizeEnvVar("1048576", "TEST_VAR", 999, 0);
  EXPECT_EQ(1048576u, result);
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: zero is accepted when minValue is 0
TEST(ShmHelpersTest, ParseShmSizeEnvVarZeroAllowed)
{
  auto result = parseShmSizeEnvVar("0", "TEST_VAR", 999, 0);
  EXPECT_EQ(0u, result);
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: zero is rejected when minValue is 1
TEST(ShmHelpersTest, ParseShmSizeEnvVarZeroRejected)
{
  testing::internal::CaptureStderr();
  auto result = parseShmSizeEnvVar("0", "TEST_VAR", 999, 1);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(999u, result);
  EXPECT_NE(std::string::npos, err.find("TEST_VAR"));
  EXPECT_NE(std::string::npos, err.find("below the minimum"));
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: negative value is rejected
TEST(ShmHelpersTest, ParseShmSizeEnvVarNegative)
{
  testing::internal::CaptureStderr();
  auto result = parseShmSizeEnvVar("-1", "TEST_VAR", 999, 0);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(999u, result);
  EXPECT_NE(std::string::npos, err.find("TEST_VAR"));
  EXPECT_NE(std::string::npos, err.find("negative"));
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: non-numeric string is rejected
TEST(ShmHelpersTest, ParseShmSizeEnvVarNonNumeric)
{
  testing::internal::CaptureStderr();
  auto result = parseShmSizeEnvVar("abc", "TEST_VAR", 999, 0);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(999u, result);
  EXPECT_NE(std::string::npos, err.find("TEST_VAR"));
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: empty string is rejected
TEST(ShmHelpersTest, ParseShmSizeEnvVarEmpty)
{
  testing::internal::CaptureStderr();
  auto result = parseShmSizeEnvVar("", "TEST_VAR", 999, 0);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(999u, result);
  EXPECT_NE(std::string::npos, err.find("TEST_VAR"));
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: overflow is rejected
TEST(ShmHelpersTest, ParseShmSizeEnvVarOverflow)
{
  testing::internal::CaptureStderr();
  auto result = parseShmSizeEnvVar(
      "99999999999999999999", "TEST_VAR", 999, 0);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(999u, result);
  EXPECT_NE(std::string::npos, err.find("TEST_VAR"));
  EXPECT_NE(std::string::npos, err.find("out of range"));
}

//////////////////////////////////////////////////
// parseShmSizeEnvVar: value below custom minimum is rejected
TEST(ShmHelpersTest, ParseShmSizeEnvVarBelowMinimum)
{
  testing::internal::CaptureStderr();
  auto result = parseShmSizeEnvVar("5", "TEST_VAR", 999, 10);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(999u, result);
  EXPECT_NE(std::string::npos, err.find("TEST_VAR"));
  EXPECT_NE(std::string::npos, err.find("below the minimum"));
}

//////////////////////////////////////////////////
// warnShmConfig: warns when pool size exceeds 1 GB
TEST(ShmHelpersTest, WarnShmConfigLargePool)
{
  ShmEnvConfig config;
  config.poolSize = 2UL * 1024 * 1024 * 1024;  // 2 GB
  config.threshold = kDefaultShmThreshold;

  testing::internal::CaptureStderr();
  warnShmConfig(config);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_NE(std::string::npos, err.find("SHM_POOL_SIZE"));
}

//////////////////////////////////////////////////
// warnShmConfig: warns when pool is smaller than threshold
TEST(ShmHelpersTest, WarnShmConfigPoolLessThanThreshold)
{
  ShmEnvConfig config;
  config.poolSize = 100;
  config.threshold = 1000;

  testing::internal::CaptureStderr();
  warnShmConfig(config);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_NE(std::string::npos, err.find("never be used"));
}

//////////////////////////////////////////////////
// warnShmConfig: no warning for normal defaults
TEST(ShmHelpersTest, WarnShmConfigNormal)
{
  ShmEnvConfig config;  // uses defaults

  testing::internal::CaptureStderr();
  warnShmConfig(config);
  std::string err = testing::internal::GetCapturedStderr();
  EXPECT_TRUE(err.empty());
}

//////////////////////////////////////////////////
// createShmProvider: returns a valid provider when enabled
TEST(ShmHelpersTest, CreateShmProvider)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);
}

//////////////////////////////////////////////////
// createShmProvider: returns nullptr when SHM is disabled
TEST(ShmHelpersTest, CreateShmProviderDisabled)
{
  // Temporarily disable SHM via the cached config.
  setShmEnabled(false);
  auto provider = createShmProvider();
  EXPECT_EQ(nullptr, provider);

  // Restore.
  setShmEnabled(true);
}

//////////////////////////////////////////////////
// allocShmBuf: returns nullopt for null provider
TEST(ShmHelpersTest, AllocShmBufNullProvider)
{
  auto result = allocShmBuf(nullptr, 1024);
  EXPECT_FALSE(result.has_value());
}

//////////////////////////////////////////////////
// allocShmBuf: returns nullopt for size below threshold
TEST(ShmHelpersTest, AllocShmBufBelowThreshold)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);

  // Threshold is 128 KB by default, so 1 byte should be below it.
  auto result = allocShmBuf(provider.get(), 1);
  EXPECT_FALSE(result.has_value());
}

//////////////////////////////////////////////////
// allocShmBuf: returns nullopt when pool is exhausted
TEST(ShmHelpersTest, AllocShmBufPoolExhausted)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);

  // Request more than the entire pool (48 MB + 1).
  const std::size_t tooLarge = shmEnvConfig().poolSize + 1;
  auto result = allocShmBuf(provider.get(), tooLarge);
  EXPECT_FALSE(result.has_value());
}

//////////////////////////////////////////////////
// allocShmBuf: returns nullopt when SHM is disabled
TEST(ShmHelpersTest, AllocShmBufDisabled)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);

  // Temporarily disable SHM via the cached config.
  setShmEnabled(false);
  auto disabledProvider = createShmProvider();
  EXPECT_EQ(nullptr, disabledProvider);

  // allocShmBuf with a null provider should return nullopt.
  auto result = allocShmBuf(disabledProvider.get(),
                             shmEnvConfig().threshold);
  EXPECT_FALSE(result.has_value());

  // Restore.
  setShmEnabled(true);
}

//////////////////////////////////////////////////
// allocShmBuf: succeeds for size at or above threshold
TEST(ShmHelpersTest, AllocShmBufAboveThreshold)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);

  const std::size_t threshold = shmEnvConfig().threshold;
  auto result = allocShmBuf(provider.get(), threshold);
  ASSERT_TRUE(result.has_value());
  EXPECT_GE(result->len(), threshold);
}

//////////////////////////////////////////////////
// allocShmBuf: data written to buffer is readable
TEST(ShmHelpersTest, AllocShmBufWriteRead)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);

  const std::size_t threshold = shmEnvConfig().threshold;
  auto result = allocShmBuf(provider.get(), threshold);
  ASSERT_TRUE(result.has_value());

  // Write a pattern and verify it reads back.
  const char pattern = 0x42;
  memset(result->data(), pattern, threshold);
  EXPECT_EQ(pattern, static_cast<char>(result->data()[0]));
  EXPECT_EQ(pattern, static_cast<char>(result->data()[threshold - 1]));
}

//////////////////////////////////////////////////
// serviceShmProvider: returns same pointer on repeated calls
TEST(ShmHelpersTest, ServiceShmProviderSingleton)
{
  auto *p1 = serviceShmProvider();
  auto *p2 = serviceShmProvider();
  EXPECT_NE(p1, nullptr);
  EXPECT_EQ(p2, p1);
}

//////////////////////////////////////////////////
// serviceShmProvider: is distinct from per-publisher providers
TEST(ShmHelpersTest, ServiceProviderDistinctFromPublisher)
{
  auto pubProvider = createShmProvider();
  auto *svcProvider = serviceShmProvider();
  ASSERT_NE(pubProvider, nullptr);
  ASSERT_NE(svcProvider, nullptr);
  EXPECT_NE(pubProvider.get(), svcProvider);
}

//////////////////////////////////////////////////
// allocShmBuf: multiple allocations from the same provider
TEST(ShmHelpersTest, AllocShmBufMultiple)
{
  auto provider = createShmProvider();
  ASSERT_NE(provider, nullptr);

  const std::size_t threshold = shmEnvConfig().threshold;
  auto buf1 = allocShmBuf(provider.get(), threshold);
  auto buf2 = allocShmBuf(provider.get(), threshold);
  ASSERT_TRUE(buf1.has_value());
  ASSERT_TRUE(buf2.has_value());

  // Buffers should be at different addresses.
  EXPECT_NE(buf1->data(), buf2->data());
}

//////////////////////////////////////////////////
// CreateMsgFromBuffer: typed SubscriptionHandler
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

#endif  // Z_FEATURE_SHARED_MEMORY
#endif  // HAVE_ZENOH

// Provide a minimal test when SHM or Zenoh is unavailable so the
// test binary still compiles and reports success.
#if !defined(HAVE_ZENOH) || \
    !defined(Z_FEATURE_SHARED_MEMORY)

TEST(ShmHelpersTest, NotAvailable)
{
  GTEST_SKIP() << "SHM helpers require Zenoh with SHM support";
}
#endif
