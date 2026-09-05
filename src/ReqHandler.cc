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

#include <iostream>
#include <memory>
#include <string>
#include "gz/transport/config.hh"
#include "gz/transport/ReqHandler.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>
#endif

namespace gz::transport
{
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
  {
  /// \internal
  /// \brief Private data for IReqHandler class.
  class IReqHandlerPrivate
  {
    /// \brief Default constructor.
    public: IReqHandlerPrivate(const std::string &_nUuid)
    : hUuid(Uuid().ToString()),
      nUuid(_nUuid),
      requested(false)
    {
    }

    /// \brief Destructor.
    public: virtual ~IReqHandlerPrivate() = default;

    /// \brief Unique handler's UUID.
    public: std::string hUuid;

    /// \brief Node UUID.
    public: std::string nUuid;

    /// \brief When true, the REQ was already sent and the REP should be on
    /// its way. Used to not resend the same REQ more than one time.
    public: bool requested;
  };

  /////////////////////////////////////////////////
  IReqHandler::IReqHandler(const std::string &_nUuid)
    : dataPtr(new IReqHandlerPrivate(_nUuid)),
      rep(""),
      result(false),
      repAvailable(false)
  {
  }

  /////////////////////////////////////////////////
  IReqHandler::~IReqHandler()
  {
  }

  /////////////////////////////////////////////////
  std::string IReqHandler::HandlerUuid() const
  {
    return this->dataPtr->hUuid;
  }

  /////////////////////////////////////////////////
  std::string IReqHandler::NodeUuid() const
  {
    return this->dataPtr->nUuid;
  }

  /////////////////////////////////////////////////
  bool IReqHandler::Requested() const
  {
    return this->dataPtr->requested;
  }

  /////////////////////////////////////////////////
  void IReqHandler::Requested(const bool _value)
  {
    this->dataPtr->requested = _value;
  }

#ifdef HAVE_ZENOH
  /////////////////////////////////////////////////
  bool IReqHandler::CreateZenohGet(
    std::shared_ptr<zenoh::Querier> _querier,
    const std::string &_service)
  {
    if (!_querier)
    {
      std::cerr << "gz-transport zenoh: no Querier for [" << _service
                << "]; aborting request.\n";
      return false;
    }

    // The reply closure holds a weak reference to this handler, so a
    // reply arriving after the handler was removed from the requests
    // storage (e.g. after Node::Request timed out) is dropped instead
    // of dereferencing a dead object.
    std::weak_ptr<IReqHandler> weakSelf = this->weak_from_this();
    if (weakSelf.expired())
    {
      std::cerr << "gz-transport zenoh: IReqHandler for [" << _service
                << "] is not owned by a shared_ptr; aborting request.\n";
      return false;
    }

    // The persistent Querier carries an always-on interest
    // declaration on _service, so the responser's queryable
    // announcement has a routing path back to this session even if
    // the responser starts after us. The query below then stays in
    // flight (bounded by the Zenoh query timeout) and reaches a
    // queryable that appears late. This is what closes the
    // cold-start race.

    // Capture _service BY VALUE: the closure can fire on a Zenoh
    // worker thread long after this stack frame has returned.
    auto onReply = [weakSelf, _service](const zenoh::Reply &_reply)
    {
      auto self = weakSelf.lock();
      if (!self)
        return;
      if (_reply.is_ok())
      {
        const auto &sample = _reply.get_ok();
        self->NotifyResult(sample.get_payload().as_string(), true);
      }
      else
      {
        std::cerr << "gz-transport zenoh: error reply on [" << _service
                  << "]: "
                  << _reply.get_err().get_payload().as_string() << "\n";
      }
    };

    zenoh::Querier::GetOptions getOpts =
      zenoh::Querier::GetOptions::create_default();
    std::string payload;
    this->Serialize(payload);
    if (!payload.empty())
      getOpts.payload = zenoh::Bytes(payload);

    // Fire and forget: the caller (Node::Request) waits on the
    // handler's condition variable via WaitUntil, mirroring the
    // ZeroMQ flow. Blocking here instead would stall the calling
    // thread while it holds NodeShared::mutex, serializing every
    // other request, subscription, and teardown in the process
    // (and would wait the user timeout twice).
    try
    {
      _querier->get("", onReply, []() {}, std::move(getOpts));
    }
    catch (const zenoh::ZException &e)
    {
      std::cerr << "gz-transport zenoh: querier.get failed for ["
                << _service << "]: " << e.what() << "\n";
      return false;
    }
    return true;
  }
#endif
  }
}
