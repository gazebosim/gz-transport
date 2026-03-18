/*
 * Copyright (C) 2025 Open Source Robotics Foundation
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

#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include "gz/transport/config.hh"
#include "gz/transport/Helpers.hh"
#include "gz/transport/RepHandler.hh"
#include "gz/transport/TopicUtils.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
// Unlock get_contiguous_view() for zero-copy SHM receive path.
#define Z_FEATURE_UNSTABLE_API
#include <zenoh.hxx>

namespace
{
  constexpr std::size_t kDefaultShmPoolSize = 10 * 1024 * 1024;

  std::size_t ShmThreshold()
  {
    static const std::size_t threshold = []() -> std::size_t {
      const char *val = std::getenv("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD");
      if (val)
      {
        try { return std::stoul(val); }
        catch (...) {}
      }
      return 128 * 1024;
    }();
    return threshold;
  }
}
#endif

namespace gz::transport
{
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
  {
  /// \internal
  /// \brief Private data for IRepHandler class.
  class IRepHandlerPrivate
  {
    /// \brief Default constructor.
    public: IRepHandlerPrivate(
      const std::string &_pUuid,
      const std::string &_nUuid)
    : pUuid(_pUuid),
      nUuid(_nUuid),
      hUuid(Uuid().ToString())
    {
#ifdef HAVE_ZENOH
      bool shmEnabled = true;
      std::string shmEnvValue;
      if (env("GZ_TRANSPORT_ZENOH_SHM", shmEnvValue) &&
          (shmEnvValue == "0" || shmEnvValue == "false"))
      {
        shmEnabled = false;
      }
      if (shmEnabled)
      {
        std::size_t poolSize = kDefaultShmPoolSize;
        std::string poolSizeStr;
        if (env("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE", poolSizeStr))
        {
          try { poolSize = std::stoul(poolSizeStr); }
          catch (...) { /* use default */ }
        }
        try
        {
          this->provider = std::make_unique<zenoh::PosixShmProvider>(
            zenoh::MemoryLayout(poolSize, zenoh::AllocAlignment({0})));
        }
        catch (const std::exception &_e)
        {
          std::cerr << "gz-transport: SHM provider creation failed ("
                    << _e.what() << "), falling back to heap.\n";
        }
      }
#endif
    }

    /// \brief Destructor.
    public: virtual ~IRepHandlerPrivate() = default;

    /// \brief Process UUID.
    public: std::string pUuid;

    /// \brief Node UUID.
    public: std::string nUuid;

    /// \brief Handler UUID.
    public: std::string hUuid;

#ifdef HAVE_ZENOH
    /// \brief Zenoh queriable to receive requests.
    std::unique_ptr<zenoh::Queryable<void>> zQueryable;

    /// \brief The liveliness token.
    public: std::unique_ptr<zenoh::LivelinessToken> zToken;

    /// \brief SHM provider for zero-copy reply payloads.
    public: std::unique_ptr<zenoh::PosixShmProvider> provider;

    /// \brief Allocate a SHM buffer of \p _size bytes, or return nullopt if
    /// SHM is disabled, the message is below threshold, or alloc fails.
    public: std::optional<zenoh::ZShmMut> AllocShmBuf(std::size_t _size)
    {
      if (!this->provider || _size < ShmThreshold())
        return std::nullopt;
      auto result = this->provider->alloc_gc_defrag_blocking(
        _size, zenoh::AllocAlignment({0}));
      if (!std::holds_alternative<zenoh::ZShmMut>(result))
        return std::nullopt;
      return std::get<zenoh::ZShmMut>(std::move(result));
    }
#endif
  };

  /////////////////////////////////////////////////
  IRepHandler::IRepHandler(const std::string &_pUuid,
      const std::string &_nUuid)
    : dataPtr(new IRepHandlerPrivate(_pUuid, _nUuid))
  {
  }

  /////////////////////////////////////////////////
  IRepHandler::~IRepHandler()
  {
  }

  /////////////////////////////////////////////////
  std::string IRepHandler::HandlerUuid() const
  {
    return this->dataPtr->hUuid;
  }

#ifdef HAVE_ZENOH
  /////////////////////////////////////////////////
  void IRepHandler::CreateZenohQueriable(
    std::shared_ptr<zenoh::Session> _session,
    const std::string &_service)
  {
    auto onQuery = [this, _service](const zenoh::Query &_query)
    {
      std::string output;
      std::string input;

      if (_query.get_payload())
      {
        // Zero-copy SHM path: get a direct pointer into the SHM buffer when
        // available, avoiding a heap copy during deserialization.
        auto view = _query.get_payload()->get().get_contiguous_view();
        if (view.has_value())
          input.assign(
            reinterpret_cast<const char *>(view->data), view->len);
        else
          input = _query.get_payload()->get().as_string();
      }

      if (this->RunCallback(input, output))
      {
        // Use SHM for large replies to avoid a serialization copy.
        bool usedShm = false;
        if (auto shmBuf = this->dataPtr->AllocShmBuf(output.size()))
        {
          memcpy(shmBuf->data(), output.data(), output.size());
          _query.reply(_service, zenoh::Bytes(std::move(*shmBuf)));
          usedShm = true;
        }
        if (!usedShm)
          _query.reply(_service, output);
      }
    };

    auto onDropQueryable = []() {};

    zenoh::Session::QueryableOptions opts;
    this->dataPtr->zQueryable = std::make_unique<zenoh::Queryable<void>>(
      _session->declare_queryable(
        _service, onQuery, onDropQueryable, std::move(opts)));

    std::string token = TopicUtils::CreateLivelinessToken(
      _service, this->dataPtr->pUuid, this->dataPtr->nUuid, "SS",
      this->ReqTypeName(), this->RepTypeName());

    if (token.empty())
      return;

    this->dataPtr->zToken = std::make_unique<zenoh::LivelinessToken>(
      _session->liveliness_declare_token(token));
  }
#endif
  }
}
