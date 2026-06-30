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

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include "gz/transport/config.hh"
#include "gz/transport/RepHandler.hh"
#include "gz/transport/TopicUtils.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>
#include "NodeSharedPrivate.hh"
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
      this->Shutdown();
    }

    /// \brief Zenoh teardown. Safe to call multiple times.
    /// See ZenohTeardownEntity in NodeSharedPrivate.hh for the
    /// shared pattern (atomic guard + detached undeclare). Running
    /// undeclare() inline would self-deadlock if a service callback
    /// itself triggers UnadvertiseSrv (which calls this teardown):
    /// the callback would wait for teardown to finish, and teardown
    /// would wait for the callback to return.
    public: void Shutdown()
    {
#ifdef HAVE_ZENOH
      ZenohTeardownEntity(this->isShutdown,
                          this->zQueryable, this->zToken);
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

    /// \brief Atomic guard for Shutdown idempotence.
    public: std::atomic<bool> isShutdown{false};

    /// \brief Temporarily store the dispatch closure passed from the header.
    public: std::function<bool(const std::string&, std::string&)> zenohDispatch;
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
  void IRepHandler::SetZenohQueryableDispatch(
      const std::string &/*_service*/,
      std::function<bool(const std::string &request,
                         std::string &response)> _dispatch)
  {
    this->dataPtr->zenohDispatch = std::move(_dispatch);
  }

  /////////////////////////////////////////////////
  void IRepHandler::CreateZenohQueriable(
    std::shared_ptr<zenoh::Session> _session,
    const std::string &_service)
  {
    if (!this->dataPtr->zenohDispatch)
    {
      std::cerr << "IRepHandler::CreateZenohQueriable: no dispatch set. "
                << "Call SetZenohQueryableDispatch first.\n";
      return;
    }
    auto onQuery =
      [_dispatch = std::move(this->dataPtr->zenohDispatch), _service](
        const zenoh::Query &_query)
    {
      std::string input = "";
      if (_query.get_payload())
        input = _query.get_payload()->get().as_string();
      std::string output;
      if (_dispatch(input, output))
        _query.reply(_service, output);
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
