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

#include <memory>
#include <string>
#include <thread>
#include "gz/transport/config.hh"
#include "gz/transport/NodeShared.hh"
#include "gz/transport/RepHandler.hh"
#include "gz/transport/TopicUtils.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>
#include "ShmHelpers.hh"
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
    }

    /// \brief Destructor.
    public: virtual ~IRepHandlerPrivate()
    {
#ifdef HAVE_ZENOH
      // When unregistering from within a Zenoh callback, destroying the
      // Queryable synchronously causes a deadlock in Zenoh's wait_callbacks()
      // because it waits for the current thread (callback worker) to finish.
      // Move them to a detached thread so the callback can return cleanly.
      if (this->zQueryable || this->zToken)
      {
        std::thread([queryable = std::move(this->zQueryable),
                     token = std::move(this->zToken)]() mutable
        {
          queryable.reset();
          token.reset();
        }).detach();
      }
#endif
    }

    /// \brief Process UUID.
    public: std::string pUuid;

    /// \brief Node UUID.
    public: std::string nUuid;

    /// \brief Handler UUID.
    public: std::string hUuid;

#ifdef HAVE_ZENOH
    /// \brief Zenoh queryable to receive requests. Persistent for
    /// the IRepHandler's lifetime so its interest declaration on
    /// the service keyexpr remains in effect.
    public: std::unique_ptr<zenoh::Queryable<void>> zQueryable;

    /// \brief The liveliness token.
    public: std::unique_ptr<zenoh::LivelinessToken> zToken;
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
    // The closure never keeps a reference to this handler. It resolves
    // it through the repliers storage, which owns every handler
    // registered by Node::Advertise, keyed by the same service, node
    // UUID and handler UUID. A query arriving after
    // Node::UnadvertiseSrv removed the handler finds nothing and is
    // dropped, and a query arriving while the handler is alive keeps it
    // alive for the duration of the callback, including when the
    // callback itself unadvertises the service. NodeShared is never
    // destroyed (see NodeShared::Instance()), so capturing the pointer
    // is safe even for queries delivered during process exit.
    NodeShared *shared = NodeShared::Instance();
    const std::string nUuid = this->dataPtr->nUuid;
    const std::string hUuid = this->dataPtr->hUuid;
    auto onQuery =
      [shared, nUuid, hUuid, _service](const zenoh::Query &_query)
    {
      IRepHandlerPtr self;
      {
        std::lock_guard<std::recursive_mutex> lk(shared->mutex);
        if (!shared->Repliers().Handler(_service, nUuid, hUuid, self))
          return;
      }

      std::string input;
      if (_query.get_payload())
      {
        // Reads through a direct pointer into the (SHM) buffer when the
        // payload is contiguous.
        input = payloadToString(_query.get_payload()->get());
      }

      std::string output;
      if (self->RunCallback(input, output))
        _query.reply(_service, output);
    };

    auto onDropQueryable = []() {};

    zenoh::Session::QueryableOptions opts;
    this->dataPtr->zQueryable = std::make_unique<zenoh::Queryable<void>>(
      _session->declare_queryable(
        _service, std::move(onQuery), onDropQueryable, std::move(opts)));

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
