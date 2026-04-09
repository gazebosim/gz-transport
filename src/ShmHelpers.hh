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
#include <stdexcept>
#include <string>
#include <utility>
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

  /// \brief Pool size above which a warning is emitted (1 GB).
  /// Each publisher allocates its own pool, so large values multiply quickly.
  constexpr std::size_t kShmPoolSizeWarningThreshold =
      1UL * 1024 * 1024 * 1024;

  /// \brief Cached SHM configuration.
  /// Pool size and threshold are read from environment variables once on
  /// first access (thread-safe via static initialization). The enabled
  /// flag is set by NodeSharedPrivate after resolving the Zenoh config
  /// (which may come from ZENOH_CONFIG file, defaults, or
  /// GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE).
  struct ShmEnvConfig
  {
    /// \brief Whether SHM is enabled.
    /// Set from the resolved Zenoh config's
    /// transport/shared_memory/enabled value (default: true).
    bool enabled = true;

    /// \brief SHM pool size in bytes.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE (default: 48 MB).
    std::size_t poolSize = kDefaultShmPoolSize;

    /// \brief Minimum message size to use SHM, in bytes.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_THRESHOLD (default: 128 KB).
    std::size_t threshold = kDefaultShmThreshold;
  };

  /// \brief Parse and validate a size_t environment variable value.
  /// Uses signed parsing (std::stol) so that negative values are detected
  /// rather than silently wrapping to huge unsigned values.
  /// \param[in] _val The string value to parse.
  /// \param[in] _envVarName Name of the env var (for error messages).
  /// \param[in] _defaultValue Value returned when parsing fails.
  /// \param[in] _minValue Minimum accepted value (inclusive).
  /// \return The parsed value, or _defaultValue on any error.
  inline std::size_t parseShmSizeEnvVar(
      const std::string &_val,
      const std::string &_envVarName,
      std::size_t _defaultValue,
      std::size_t _minValue)
  {
    long numVal;
    try
    {
      numVal = std::stol(_val);
    }
    catch (std::invalid_argument &)
    {
      std::cerr << "Unable to convert " << _envVarName << " value ["
                << _val << "] to an integer number. Using ["
                << _defaultValue << "] instead." << std::endl;
      return _defaultValue;
    }
    catch (std::out_of_range &)
    {
      std::cerr << "Unable to convert " << _envVarName << " value ["
                << _val << "] to an integer number. This number is "
                << "out of range. Using [" << _defaultValue
                << "] instead." << std::endl;
      return _defaultValue;
    }

    if (numVal < 0)
    {
      std::cerr << "Unable to convert " << _envVarName << " value ["
                << _val << "] to a non-negative number. This number is "
                << "negative. Using [" << _defaultValue
                << "] instead." << std::endl;
      return _defaultValue;
    }

    auto result = static_cast<std::size_t>(numVal);
    if (result < _minValue)
    {
      std::cerr << _envVarName << " value [" << _val
                << "] is below the minimum (" << _minValue
                << "). Using [" << _defaultValue
                << "] instead." << std::endl;
      return _defaultValue;
    }

    return result;
  }

  /// \brief Emit warnings for suspicious SHM configuration combinations.
  /// \param[in] _config The configuration to check.
  inline void warnShmConfig(const ShmEnvConfig &_config)
  {
    if (_config.poolSize > kShmPoolSizeWarningThreshold)
    {
      std::cerr << "gz-transport: GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE is "
                << _config.poolSize << " bytes (>"
                << kShmPoolSizeWarningThreshold
                << "). Each publisher allocates its own pool."
                << std::endl;
    }

    if (_config.poolSize < _config.threshold)
    {
      std::cerr << "gz-transport: GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE ("
                << _config.poolSize
                << ") is smaller than GZ_TRANSPORT_ZENOH_SHM_THRESHOLD ("
                << _config.threshold
                << "). SHM will effectively never be used."
                << std::endl;
    }
  }

  /// \brief Get the cached SHM configuration.
  /// Environment variables are read once on first call.
  /// \return Reference to the process-wide SHM configuration.
  inline ShmEnvConfig &shmEnvConfig()
  {
    static ShmEnvConfig config = []()
    {
      ShmEnvConfig c;
      std::string val;

      if (env("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE", val))
      {
        c.poolSize = parseShmSizeEnvVar(
            val, "GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE",
            kDefaultShmPoolSize, 1);
      }

      if (env("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD", val))
      {
        c.threshold = parseShmSizeEnvVar(
            val, "GZ_TRANSPORT_ZENOH_SHM_THRESHOLD",
            kDefaultShmThreshold, 0);
      }

      warnShmConfig(c);
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
  inline zenoh::PosixShmProvider* serviceShmProvider()
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
  inline std::nullptr_t serviceShmProvider()
  {
    return nullptr;
  }

#endif  // Z_FEATURE_SHARED_MEMORY && Z_FEATURE_UNSTABLE_API

}  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport

#endif  // HAVE_ZENOH
#endif  // GZ_TRANSPORT_SHMHELPERS_HH_
