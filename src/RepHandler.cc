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
#include "gz/transport/config.hh"
#include "gz/transport/RepHandler.hh"
#include "gz/transport/Uuid.hh"

#ifdef HAVE_ZENOH
#include <zenoh.hxx>
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
    public: IRepHandlerPrivate()
    : hUuid(Uuid().ToString())
    {
    }

    /// \brief Destructor.
    public: virtual ~IRepHandlerPrivate() = default;

    /// \brief Subscribe options.
    public: std::string hUuid;

#ifdef HAVE_ZENOH
    /// \brief Zenoh queriable to receive requests.
    std::unique_ptr<zenoh::Queryable<void>> zQueryable;
#endif
  };

  /////////////////////////////////////////////////
  IRepHandler::IRepHandler()
    : dataPtr(new IRepHandlerPrivate())
  {
  }

  /////////////////////////////////////////////////
  IRepHandler::~IRepHandler()
  {
    if (dataPtr)
      delete dataPtr;
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
      this->RunCallback(_query.get_payload()->get().as_string(), output);
      _query.reply(_service, output);
    };

    auto onDropQueryable = []() {};

    zenoh::Session::QueryableOptions opts;
    this->dataPtr->zQueryable = std::make_unique<zenoh::Queryable<void>>(
      _session->declare_queryable(
        _service, onQuery, onDropQueryable, std::move(opts)));
  }
#endif
  }
}
