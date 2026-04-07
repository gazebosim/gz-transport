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

#ifndef GZ_TRANSPORT_SHMHELPERS_HH_
#define GZ_TRANSPORT_SHMHELPERS_HH_

#include "gz/transport/config.hh"

#ifdef HAVE_ZENOH

#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

#include "gz/transport/Helpers.hh"

#include <zenoh.hxx>

namespace gz::transport
{
inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
{
// SHM requires POSIX shared memory. Zenoh defines Z_FEATURE_SHARED_MEMORY
// when the feature is compiled in (Linux, macOS). On platforms without SHM
// support (e.g. Windows) all helpers return nullptr / nullopt so the code
// falls back to heap-based transfer transparently.
#if defined(Z_FEATURE_SHARED_MEMORY) && defined(Z_FEATURE_UNSTABLE_API)

  /// \brief Default SHM pool size (48 MB, matches rmw_zenoh default).
  constexpr std::size_t kDefaultShmPoolSize = 48 * 1024 * 1024;

  /// \brief Default SHM threshold (128 KB).
  /// Messages below this size use heap-based transfer, which is safer for
  /// short-lived publishers: zenoh copies heap data internally, so the
  /// subscriber still receives it after the publisher exits. SHM buffers
  /// are reclaimed when the publisher's pool is destroyed.
  constexpr std::size_t kDefaultShmThreshold = 128 * 1024;

  /// \brief Cached SHM configuration read from environment variables.
  /// All values are read once on first access (thread-safe via static
  /// initialization). This ensures consistent behavior even if env vars
  /// are modified after the first read.
  struct ShmEnvConfig
  {
    /// \brief Whether SHM is enabled.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_ENABLED (default: true).
    bool enabled = true;

    /// \brief SHM pool size in bytes.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE (default: 48 MB).
    std::size_t poolSize = kDefaultShmPoolSize;

    /// \brief Minimum message size to use SHM, in bytes.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_THRESHOLD (default: 128 KB).
    std::size_t threshold = kDefaultShmThreshold;
  };

  /// \brief Get the cached SHM configuration.
  /// All environment variables are read once on first call.
  /// \return Reference to the process-wide SHM configuration.
  inline const ShmEnvConfig &shmEnvConfig()
  {
    static const ShmEnvConfig config = []()
    {
      ShmEnvConfig c;
      std::string val;

      if (env("GZ_TRANSPORT_ZENOH_SHM_ENABLED", val))
        c.enabled = (val != "0" && val != "false");

      if (env("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE", val))
      {
        try { c.poolSize = std::stoul(val); }
        catch (...) {}
      }

      if (env("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD", val))
      {
        try { c.threshold = std::stoul(val); }
        catch (...) {}
      }

      return c;
    }();
    return config;
  }

  /// \brief Create a PosixShmProvider using the cached SHM configuration.
  /// \return A new provider, or nullptr if SHM is disabled or creation fails.
  inline std::unique_ptr<zenoh::PosixShmProvider> createShmProvider()
  {
    const auto &config = shmEnvConfig();
    if (!config.enabled)
      return nullptr;

    try
    {
      // AllocAlignment({0}) = 2^0 = 1-byte alignment.
      // Serialized protobuf data has no alignment requirements.
      return std::make_unique<zenoh::PosixShmProvider>(
        zenoh::MemoryLayout(config.poolSize, zenoh::AllocAlignment({0})));
    }
    catch (const std::exception &e)
    {
      std::cerr << "gz-transport: SHM provider creation failed ("
                << e.what() << "), falling back to heap.\n";
      return nullptr;
    }
  }

  /// \brief Attempt to allocate a SHM buffer for a message.
  /// Uses non-blocking allocation with GC and defragmentation.
  /// \param[in] _provider The SHM provider to allocate from.
  /// \param[in] _size Number of bytes to allocate.
  /// \return The SHM buffer, or std::nullopt if SHM is disabled, the message
  /// is below threshold, or allocation fails.
  inline std::optional<zenoh::ZShmMut> allocShmBuf(
      zenoh::PosixShmProvider *_provider, std::size_t _size)
  {
    if (!_provider || _size < shmEnvConfig().threshold)
      return std::nullopt;

    // Non-blocking alloc with garbage collection and defragmentation.
    // AllocAlignment({0}) = 2^0 = 1-byte alignment for serialized data.
    auto result = _provider->alloc_gc_defrag(
      _size, zenoh::AllocAlignment({0}));

    if (!std::holds_alternative<zenoh::ZShmMut>(result))
      return std::nullopt;

    return std::get<zenoh::ZShmMut>(std::move(result));
  }

  /// \brief Get the process-level SHM provider shared by all service handlers.
  /// Thread-safe (uses std::call_once). Both ReqHandler and RepHandler share
  /// this pool, which avoids memory explosion with many service advertisers.
  /// \return The shared provider, or nullptr if SHM is disabled or unavailable.
  inline zenoh::PosixShmProvider* getServiceShmProvider()
  {
    static std::unique_ptr<zenoh::PosixShmProvider> provider;
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
    {
      provider = createShmProvider();
    });
    return provider.get();
  }

#else  // No SHM support — provide no-op fallbacks so call sites compile
       // without extra #ifdefs. All functions return nullptr/nullopt,
       // causing transparent fallback to heap-based transfer.

  /// \brief No-op: SHM not available on this platform.
  /// \return Always returns nullptr.
  inline std::unique_ptr<std::nullptr_t> createShmProvider()
  {
    return nullptr;
  }

  /// \brief No-op: SHM not available on this platform.
  /// \return Always returns std::nullopt.
  inline std::nullopt_t allocShmBuf(std::nullptr_t, std::size_t)
  {
    return std::nullopt;
  }

  /// \brief No-op: SHM not available on this platform.
  /// \return Always returns nullptr.
  inline std::nullptr_t getServiceShmProvider()
  {
    return nullptr;
  }

#endif  // Z_FEATURE_SHARED_MEMORY && Z_FEATURE_UNSTABLE_API

}  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport

#endif  // HAVE_ZENOH
#endif  // GZ_TRANSPORT_SHMHELPERS_HH_
