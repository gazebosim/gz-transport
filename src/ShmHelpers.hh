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

#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

#include "gz/transport/config.hh"
#include "gz/transport/Helpers.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>

namespace gz::transport
{
inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
{
  /// \brief Default SHM pool size (48 MB, matches rmw_zenoh default).
  constexpr std::size_t kDefaultShmPoolSize = 48 * 1024 * 1024;

  /// \brief Default SHM threshold (128 KB).
  /// Messages below this size use heap-based transfer, which is safer for
  /// short-lived publishers: zenoh copies heap data internally, so the
  /// subscriber still receives it after the publisher exits. SHM buffers
  /// are reclaimed when the publisher's pool is destroyed.
  constexpr std::size_t kDefaultShmThreshold = 128 * 1024;

  /// \brief Get the SHM threshold (minimum message size to use SHM).
  /// Reads GZ_TRANSPORT_ZENOH_SHM_THRESHOLD on first call, caches result.
  /// Default: 128 KB.
  inline std::size_t ShmThreshold()
  {
    static const std::size_t threshold = []() -> std::size_t {
      std::string val;
      if (env("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD", val))
      {
        try { return std::stoul(val); }
        catch (...) {}
      }
      return kDefaultShmThreshold;
    }();
    return threshold;
  }

  /// \brief Create a PosixShmProvider if SHM is enabled.
  /// Reads GZ_TRANSPORT_ZENOH_SHM_ENABLED and GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE.
  /// Returns nullptr if SHM is disabled or creation fails.
  inline std::unique_ptr<zenoh::PosixShmProvider> CreateShmProvider()
  {
    std::string shmEnvValue;
    if (env("GZ_TRANSPORT_ZENOH_SHM_ENABLED", shmEnvValue) &&
        (shmEnvValue == "0" || shmEnvValue == "false"))
    {
      return nullptr;
    }

    static const std::size_t poolSize = []() -> std::size_t {
      std::string val;
      if (env("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE", val))
      {
        try { return std::stoul(val); }
        catch (...) {}
      }
      return kDefaultShmPoolSize;
    }();

    try
    {
      // AllocAlignment({0}) = 2^0 = 1-byte alignment.
      // Serialized protobuf data has no alignment requirements.
      return std::make_unique<zenoh::PosixShmProvider>(
        zenoh::MemoryLayout(poolSize, zenoh::AllocAlignment({0})));
    }
    catch (const std::exception &e)
    {
      std::cerr << "gz-transport: SHM provider creation failed ("
                << e.what() << "), falling back to heap.\n";
      return nullptr;
    }
  }

  /// \brief Attempt to allocate a SHM buffer for a message.
  /// Returns std::nullopt if SHM is disabled, below threshold, or alloc fails.
  /// Uses non-blocking allocation with GC and defragmentation.
  inline std::optional<zenoh::ZShmMut> AllocShmBuf(
      zenoh::PosixShmProvider *_provider, std::size_t _size)
  {
    if (!_provider || _size < ShmThreshold())
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
  /// Returns nullptr if SHM is disabled or unavailable. Thread-safe (uses
  /// std::call_once). Both ReqHandler and RepHandler share this pool, which
  /// avoids memory explosion with many service advertisers.
  inline zenoh::PosixShmProvider* GetServiceShmProvider()
  {
    static std::unique_ptr<zenoh::PosixShmProvider> provider;
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
    {
      provider = CreateShmProvider();
    });
    return provider.get();
  }

}  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport

#endif  // HAVE_ZENOH
#endif  // GZ_TRANSPORT_SHMHELPERS_HH_
