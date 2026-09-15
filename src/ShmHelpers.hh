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
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <zenoh.hxx>

namespace gz::transport
{
inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
{
  /// \brief Zenoh config key holding the size in bytes of the SHM pool the
  /// session creates for its transport optimization. gz-transport draws
  /// its explicit SHM buffers from that same pool (see ZenohShm).
  constexpr const char kZenohShmPoolSizeKey[] =
      "transport/shared_memory/transport_optimization/pool_size";

  /// \brief Zenoh config key holding the minimum payload size in bytes
  /// that travels through SHM. gz-transport uses the same threshold for
  /// its explicit SHM buffers, so there is a single knob.
  constexpr const char kZenohShmThresholdKey[] =
      "transport/shared_memory/transport_optimization/message_size_threshold";

  /// \brief SHM pool size gz-transport writes into its built-in default
  /// Zenoh configuration (48 MiB, the same value rmw_zenoh ships). Zenoh's
  /// own default of 16 MiB is exhausted by a couple of multi-megabyte
  /// messages in flight, after which payloads silently fall back to the
  /// network path. A ZENOH_CONFIG file or GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE
  /// still wins over this value.
  constexpr std::size_t kDefaultZenohShmPoolSize = 48u * 1024u * 1024u;

  /// \brief Threshold assumed when kZenohShmThresholdKey cannot be read
  /// from the configuration (Zenoh's default).
  constexpr std::size_t kDefaultZenohShmThreshold = 3072u;

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

  /// \brief Copy a payload into a std::string, reading through a direct
  /// pointer into the buffer when the payload is contiguous.
  /// \param[in] _payload The payload to copy.
  /// \return The payload bytes.
  inline std::string payloadToString(const zenoh::Bytes &_payload)
  {
    return withPayloadView(_payload,
      [](const char *_data, std::size_t _size)
      {
        return std::string(_data, _size);
      });
  }

// The explicit SHM path requires a zenoh-c built with shared memory and the
// unstable API (ZENOHC_BUILD_WITH_SHARED_MEMORY and
// ZENOHC_BUILD_WITH_UNSTABLE_API). zenoh_configure.h then defines
// Z_FEATURE_SHARED_MEMORY and Z_FEATURE_UNSTABLE_API, and the zenoh-cpp
// headers expose the SHM API only when both are defined. The backend is
// platform independent since Zenoh 1.4 (POSIX shared memory on Linux, macOS
// and BSD, file mappings on Windows). When either macro is missing, the
// #else branch below provides no-op stand-ins with the same interface so
// call sites compile unchanged and transparently fall back to heap-based
// transfer (Zenoh's own transport optimization still applies at the
// transport level when the library supports it). Zenoh SHM types never leak
// out of this block: the public surface is ZenohShm, ShmChunk,
// allocShmChunk, makeShmBytes, and zenoh::Bytes (which exists in every
// Zenoh build).
#if defined(Z_FEATURE_SHARED_MEMORY) && defined(Z_FEATURE_UNSTABLE_API)

  /// \brief Access to the SHM provider of the Zenoh session. Owned by
  /// NodeSharedPrivate next to the session it belongs to.
  ///
  /// The session's runtime owns one SHM pool, sized by kZenohShmPoolSizeKey,
  /// that Zenoh already uses to move payloads above kZenohShmThresholdKey
  /// through shared memory (transport optimization). Instead of creating a
  /// second pool, gz-transport borrows that provider through
  /// zenoh::Session::obtain_shm_provider(), like rmw_zenoh does, and
  /// serializes directly into it. This keeps a single pool per process,
  /// a single set of configuration keys, and no SHM resources owned by
  /// gz-transport that could outlive the session at exit.
  ///
  /// Zenoh initializes the provider lazily (transport/shared_memory/mode
  /// "lazy", the default) and concurrently; until it is ready, or when SHM
  /// is disabled in the configuration, Provider() returns nullptr and the
  /// caller falls back to the heap path.
  class ZenohShm
  {
    /// \brief Attach the session whose provider is used. Called by
    /// NodeSharedPrivate right after opening the session.
    /// \param[in] _session The Zenoh session. Kept alive here so the
    /// provider handle never outlives it.
    /// \param[in] _threshold Minimum payload size, in bytes, that uses SHM.
    public: void Init(std::shared_ptr<zenoh::Session> _session,
                      std::size_t _threshold)
    {
      std::lock_guard<std::mutex> lock(this->mutex);
      this->provider.reset();
      this->session = std::move(_session);
      this->threshold.store(_threshold, std::memory_order_relaxed);
      this->state.store(this->session ? State::kUnknown : State::kDisabled,
                        std::memory_order_release);
    }

    /// \brief Minimum payload size, in bytes, that uses SHM.
    /// \return The threshold.
    public: std::size_t Threshold() const
    {
      return this->threshold.load(std::memory_order_relaxed);
    }

    /// \brief Get the session's SHM provider.
    /// \return The provider, or nullptr when no session is attached, SHM
    /// is disabled in the configuration, its initialization failed, or
    /// the provider is still initializing (callers fall back to the heap
    /// path and retry on the next allocation).
    public: const zenoh::ShmProvider *Provider()
    {
      State current = this->state.load(std::memory_order_acquire);
      if (current == State::kReady)
        return &this->provider->shm_provider();
      if (current == State::kDisabled)
        return nullptr;

      std::lock_guard<std::mutex> lock(this->mutex);
      // Another thread may have resolved the state while we waited.
      current = this->state.load(std::memory_order_relaxed);
      if (current == State::kReady)
        return &this->provider->shm_provider();
      if (current == State::kDisabled)
        return nullptr;

      auto result = this->session->obtain_shm_provider();
      if (auto *ready = std::get_if<zenoh::SharedShmProvider>(&result))
      {
        this->provider.emplace(std::move(*ready));
        this->state.store(State::kReady, std::memory_order_release);
        return &this->provider->shm_provider();
      }

      // Disabled by configuration, or initialization failed for good
      // (e.g. /dev/shm cannot hold the pool): stop asking. Only
      // SHM_PROVIDER_INITIALIZING is transient; the call above triggered
      // or joined the initialization, so try again on the next allocation.
      if (std::get<zenoh::ShmProviderNotReadyState>(result) !=
          zenoh::ShmProviderNotReadyState::SHM_PROVIDER_INITIALIZING)
      {
        this->state.store(State::kDisabled, std::memory_order_release);
      }
      return nullptr;
    }

    /// \brief Resolution state of the provider.
    private: enum class State
    {
      /// \brief Not resolved yet (no session, or still initializing).
      kUnknown,
      /// \brief The provider is available.
      kReady,
      /// \brief SHM is disabled in the configuration or failed to
      /// initialize; never retried.
      kDisabled
    };

    /// \brief Serializes provider resolution.
    private: std::mutex mutex;

    /// \brief Current state. Written under mutex, read lock-free.
    private: std::atomic<State> state{State::kDisabled};

    /// \brief Minimum payload size that uses SHM.
    private: std::atomic<std::size_t> threshold{kDefaultZenohShmThreshold};

    /// \brief The session owning the provider.
    private: std::shared_ptr<zenoh::Session> session;

    /// \brief Handle to the session's provider once resolved.
    private: std::optional<zenoh::SharedShmProvider> provider;
  };

  /// \brief Attempt to allocate a SHM buffer for a message.
  /// Uses non-blocking allocation with GC and defragmentation.
  /// \param[in] _provider The SHM provider to allocate from.
  /// \param[in] _size Number of bytes to allocate.
  /// \return The SHM buffer, or std::nullopt if _provider is null or the
  /// allocation fails (e.g. pool exhausted).
  inline std::optional<zenoh::ZShmMut> allocShmBuf(
      const zenoh::ShmProvider *_provider, std::size_t _size)
  {
    if (!_provider)
      return std::nullopt;

    // Serialized protobuf data has no alignment requirements.
    auto result = _provider->alloc_gc_defrag(_size);
    if (!std::holds_alternative<zenoh::ZShmMut>(result))
      return std::nullopt;

    return std::get<zenoh::ZShmMut>(std::move(result));
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

  /// \brief Attempt to allocate a writable SHM chunk from the session's
  /// pool. The threshold is checked before touching the provider.
  /// \param[in] _shm The SHM state of the session.
  /// \param[in] _size Number of bytes to allocate.
  /// \return The chunk, empty if SHM is disabled or not ready, the message
  /// is below threshold, or allocation fails.
  inline ShmChunk allocShmChunk(ZenohShm &_shm, std::size_t _size)
  {
    if (_size < _shm.Threshold())
      return ShmChunk();

    if (auto shmBuf = allocShmBuf(_shm.Provider(), _size))
      return ShmChunk(std::move(*shmBuf));
    return ShmChunk();
  }

  /// \brief Attempt to copy data into a fresh SHM buffer from the session's
  /// pool, wrapped in zenoh::Bytes ready for zero-copy publication.
  /// The threshold is checked before touching the provider.
  /// \param[in] _shm The SHM state of the session.
  /// \param[in] _data Pointer to the data to copy.
  /// \param[in] _size Number of bytes in _data.
  /// \return The bytes, or std::nullopt if SHM is disabled or not ready,
  /// the message is below threshold, or allocation fails.
  inline std::optional<zenoh::Bytes> makeShmBytes(
      ZenohShm &_shm, const void *_data, std::size_t _size)
  {
    if (_size < _shm.Threshold())
      return std::nullopt;

    auto shmBuf = allocShmBuf(_shm.Provider(), _size);
    if (!shmBuf)
      return std::nullopt;

    memcpy(shmBuf->data(), _data, _size);
    return zenoh::Bytes(std::move(*shmBuf));
  }

#else  // No SHM support: no-op stand-ins with the same interface so call
       // sites compile without extra #ifdefs. Allocation always fails,
       // causing transparent fallback to heap-based transfer. The branches
       // that would consume a SHM buffer still type-check (ShmChunk::Data()
       // returns nullptr and TakeBytes() returns empty zenoh::Bytes) but
       // are never taken at runtime.

  /// \brief Stand-in SHM state: nothing to attach to.
  class ZenohShm
  {
    /// \brief No-op: SHM not available in this build.
    public: void Init([[maybe_unused]] std::shared_ptr<zenoh::Session>,
                      [[maybe_unused]] std::size_t)
    {
    }
  };

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
  inline ShmChunk allocShmChunk(ZenohShm &, std::size_t)
  {
    return ShmChunk();
  }

  /// \brief No-op: SHM not available in this build.
  /// \return Always returns std::nullopt.
  inline std::optional<zenoh::Bytes> makeShmBytes(
      ZenohShm &, const void *, std::size_t)
  {
    return std::nullopt;
  }

#endif  // Z_FEATURE_SHARED_MEMORY && Z_FEATURE_UNSTABLE_API

}  // namespace GZ_TRANSPORT_VERSION_NAMESPACE
}  // namespace gz::transport

#endif  // HAVE_ZENOH
#endif  // GZ_TRANSPORT_SHMHELPERS_HH_
