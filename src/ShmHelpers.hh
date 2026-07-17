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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
  /// \brief Invoke _func with a view of the payload data.
  /// When the payload is contiguous (e.g. a SHM buffer) _func receives a
  /// zero-copy pointer into it; otherwise the payload is copied into a
  /// temporary string first (non-SHM or fragmented buffer).
  /// The view is only valid for the duration of the call.
  /// \param[in] _payload The payload to read.
  /// \param[in] _func Callable taking (const char *_data, std::size_t _size).
  /// \return Whatever _func returns.
  template <typename FuncT>
  auto withPayloadView(const zenoh::Bytes &_payload, FuncT &&_func)
  {
#if defined(Z_FEATURE_UNSTABLE_API)
    auto view = _payload.get_contiguous_view();
    if (view.has_value())
    {
      return _func(
        reinterpret_cast<const char *>(view->data), view->len);
    }
#endif
    const std::string data = _payload.as_string();
    return _func(data.data(), data.size());
  }

// SHM requires POSIX shared memory and Zenoh's unstable API. Zenoh defines
// Z_FEATURE_SHARED_MEMORY when the feature is compiled in (Linux, macOS)
// and Z_FEATURE_UNSTABLE_API when the unstable API is exposed. When either
// is missing, the #else branch below provides no-op stand-ins with the same
// interface so call sites compile unchanged and transparently fall back to
// heap-based transfer. Zenoh SHM types never leak out of this block: the
// public surface is ShmProviderPtr, ShmChunk, and zenoh::Bytes (which
// exists in every Zenoh build).
#if defined(Z_FEATURE_SHARED_MEMORY) && defined(Z_FEATURE_UNSTABLE_API)

  /// \brief Default SHM pool size (48 MB, matches rmw_zenoh default).
  /// A single pool is shared by all publishers and service handlers in
  /// the process.
  constexpr std::size_t kDefaultShmPoolSize = 48 * 1024 * 1024;

  /// \brief Default SHM threshold (128 KB).
  /// Messages below this size use heap-based transfer, which is safer for
  /// short-lived publishers: zenoh copies heap data internally, so the
  /// subscriber still receives it after the publisher exits. SHM buffers
  /// are reclaimed when the process's pool is destroyed.
  constexpr std::size_t kDefaultShmThreshold = 128 * 1024;

  /// \brief Pool size above which a warning is emitted (1 GB).
  constexpr std::size_t kShmPoolSizeWarningThreshold =
      1UL * 1024 * 1024 * 1024;

  /// \brief Whether SHM is enabled, process-wide.
  /// Set by NodeSharedPrivate (via setShmEnabled) after resolving the
  /// Zenoh config's transport/shared_memory/enabled value (which may come
  /// from ZENOH_CONFIG file, defaults, or
  /// GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE). Default: enabled.
  /// \return Reference to the flag.
  inline std::atomic<bool> &shmEnabled()
  {
    static std::atomic<bool> enabled{true};
    return enabled;
  }

  /// \brief Cached SHM configuration.
  /// Pool size and threshold are read from environment variables once on
  /// first access (thread-safe via static initialization).
  struct ShmEnvConfig
  {
    /// \brief SHM pool size in bytes.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE (default: 48 MB).
    std::size_t poolSize = kDefaultShmPoolSize;

    /// \brief Minimum message size to use SHM, in bytes.
    /// Read from GZ_TRANSPORT_ZENOH_SHM_THRESHOLD (default: 128 KB).
    std::size_t threshold = kDefaultShmThreshold;

    /// \brief True when the user explicitly configured SHM through the
    /// GZ_TRANSPORT_ZENOH_SHM_* environment variables. Controls whether
    /// a failure to create a SHM provider is reported on stderr: with
    /// SHM enabled by default, environments without SHM support (e.g.
    /// containers with a small /dev/shm) fall back to heap silently.
    bool explicitlyConfigured = false;
  };

  /// \brief Parse and validate a size_t environment variable value.
  /// Uses signed parsing (std::stoll) so that negative values are detected
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
    int64_t numVal;
    try
    {
      numVal = static_cast<int64_t>(std::stoll(_val));
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
                << "). The entire pool is backed by /dev/shm."
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
  /// \return Const reference to the process-wide SHM configuration.
  inline const ShmEnvConfig &shmEnvConfig()
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
        c.explicitlyConfigured = true;
      }

      if (env("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD", val))
      {
        c.threshold = parseShmSizeEnvVar(
            val, "GZ_TRANSPORT_ZENOH_SHM_THRESHOLD",
            kDefaultShmThreshold, 0);
        c.explicitlyConfigured = true;
      }

      warnShmConfig(c);
      return c;
    }();
    return config;
  }

  /// \brief Set the process-wide SHM enabled flag.
  /// Called during NodeSharedPrivate initialization after resolving the
  /// Zenoh config. Must run before the first SHM allocation: the shared
  /// pool is created lazily on first use and caches the flag's value.
  /// \param[in] _enabled Whether SHM should be enabled.
  inline void setShmEnabled(bool _enabled)
  {
    shmEnabled().store(_enabled, std::memory_order_relaxed);
  }

  /// \brief Create a PosixShmProvider using the cached SHM configuration.
  /// When creation fails (e.g. containers whose /dev/shm cannot hold the
  /// pool) the caller transparently falls back to heap-based transfer. A
  /// warning is emitted only once per process, and only when the user
  /// explicitly configured SHM via the GZ_TRANSPORT_ZENOH_SHM_* variables,
  /// so default setups degrade silently without polluting stderr (which
  /// tools like `gz topic` expect to stay clean).
  /// \return A new provider, or nullptr if SHM is disabled or creation fails.
  inline std::unique_ptr<zenoh::PosixShmProvider> createShmProvider()
  {
    const auto &config = shmEnvConfig();
    if (!shmEnabled().load(std::memory_order_relaxed))
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
      if (config.explicitlyConfigured)
      {
        static std::once_flag warnFlag;
        std::call_once(warnFlag, [&e]()
        {
          std::cerr << "gz-transport: SHM provider creation failed ("
                    << e.what() << "), falling back to heap.\n";
        });
      }
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

  /// \brief Get the single SHM provider shared by the whole process:
  /// all publishers plus all service request and reply handlers draw
  /// from this one pool, so memory use stays bounded regardless of how
  /// many publishers or service handlers exist. Created lazily on first
  /// use (a process that never sends a message at or above the SHM
  /// threshold never creates the pool). Thread-safe: creation uses
  /// std::call_once, and concurrent allocations are synchronized inside
  /// zenoh (rmw_zenoh shares one provider per context the same way).
  /// \return The shared provider, or nullptr if SHM is disabled or
  /// unavailable.
  inline zenoh::PosixShmProvider* processShmProvider()
  {
    static std::unique_ptr<zenoh::PosixShmProvider> provider;
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
    {
      provider = createShmProvider();
    });
    return provider.get();
  }

  /// \brief A writable SHM buffer that can be serialized into directly and
  /// then converted to zenoh::Bytes for zero-copy publication.
  /// Evaluates to false when no buffer is held (allocation failed, message
  /// below threshold, or SHM disabled/unavailable).
  class ShmChunk
  {
    /// \brief Construct an empty chunk.
    public: ShmChunk() = default;

    /// \brief Construct a chunk owning a SHM buffer.
    /// \param[in] _buf The SHM buffer to own.
    public: explicit ShmChunk(zenoh::ZShmMut &&_buf)
      : buf(std::move(_buf))
    {
    }

    /// \brief Whether this chunk holds a SHM buffer.
    public: explicit operator bool() const
    {
      return this->buf.has_value();
    }

    /// \brief Get a writable pointer to the buffer data.
    /// \return The data pointer, or nullptr when empty.
    public: uint8_t *Data()
    {
      return this->buf ? this->buf->data() : nullptr;
    }

    /// \brief Convert the buffer into zenoh::Bytes, leaving this empty.
    /// \return The bytes wrapping the SHM buffer.
    public: zenoh::Bytes TakeBytes()
    {
      zenoh::Bytes bytes(std::move(*this->buf));
      this->buf.reset();
      return bytes;
    }

    /// \brief The owned SHM buffer, if any.
    private: std::optional<zenoh::ZShmMut> buf;
  };

  /// \brief Attempt to allocate a writable SHM chunk from the shared
  /// process pool.
  /// The threshold is checked before touching the pool so that processes
  /// that never reach it never create the pool.
  /// \param[in] _size Number of bytes to allocate.
  /// \return The chunk, empty if SHM is disabled, the message is below
  /// threshold, or allocation fails.
  inline ShmChunk allocShmChunk(std::size_t _size)
  {
    if (_size < shmEnvConfig().threshold)
      return ShmChunk();

    if (auto shmBuf = allocShmBuf(processShmProvider(), _size))
      return ShmChunk(std::move(*shmBuf));
    return ShmChunk();
  }

  /// \brief Attempt to copy data into a fresh SHM buffer from the shared
  /// process pool, wrapped in zenoh::Bytes ready for zero-copy
  /// publication.
  /// The threshold is checked before touching the pool so that processes
  /// that never reach it never create the pool.
  /// \param[in] _data Pointer to the data to copy.
  /// \param[in] _size Number of bytes in _data.
  /// \return The bytes, or std::nullopt if SHM is disabled, the message is
  /// below threshold, or allocation fails.
  inline std::optional<zenoh::Bytes> makeShmBytes(
      const void *_data, std::size_t _size)
  {
    if (_size < shmEnvConfig().threshold)
      return std::nullopt;

    auto shmBuf = allocShmBuf(processShmProvider(), _size);
    if (!shmBuf)
      return std::nullopt;

    memcpy(shmBuf->data(), _data, _size);
    return zenoh::Bytes(std::move(*shmBuf));
  }

#else  // No SHM support — no-op stand-ins with the same interface so call
       // sites compile without extra #ifdefs. Allocation always fails,
       // causing transparent fallback to heap-based transfer. The branches
       // that would consume a SHM buffer still type-check (ShmChunk::Data()
       // returns nullptr and TakeBytes() returns empty zenoh::Bytes) but
       // are never taken at runtime.

  /// \brief No-op: SHM not available in this build.
  inline void setShmEnabled([[maybe_unused]] bool _enabled)
  {
  }

  /// \brief Stand-in chunk: always empty.
  class ShmChunk
  {
    /// \brief Whether this chunk holds a SHM buffer. Always false.
    public: explicit operator bool() const
    {
      return false;
    }

    /// \brief Get a writable pointer to the buffer data. Always nullptr.
    public: uint8_t *Data()
    {
      return nullptr;
    }

    /// \brief Convert the buffer into zenoh::Bytes. Never called at
    /// runtime; returns empty bytes so dependent code type-checks.
    public: zenoh::Bytes TakeBytes()
    {
      return zenoh::Bytes();
    }
  };

  /// \brief No-op: SHM not available in this build.
  /// \return Always returns an empty chunk.
  inline ShmChunk allocShmChunk(std::size_t)
  {
    return ShmChunk();
  }

  /// \brief No-op: SHM not available in this build.
  /// \return Always returns std::nullopt.
  inline std::optional<zenoh::Bytes> makeShmBytes(
      const void *, std::size_t)
  {
    return std::nullopt;
  }

#endif  // Z_FEATURE_SHARED_MEMORY && Z_FEATURE_UNSTABLE_API

}  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport

#endif  // HAVE_ZENOH
#endif  // GZ_TRANSPORT_SHMHELPERS_HH_
