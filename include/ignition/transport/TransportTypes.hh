/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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
#ifndef _IGN_TRANSPORT_TRANSPORTTYPES_HH_INCLUDED__
#define _IGN_TRANSPORT_TRANSPORTTYPES_HH_INCLUDED__

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ignition/transport/Publisher.hh"

namespace ignition
{
  namespace transport
  {
    /// \brief Forward declarations.
    class IRepHandler;
    class IReqHandler;
    class ISubscriptionHandler;

    /// \def MsgAddresses_M
    /// \brief The map stores all the publishers advertising this topic.
    /// The keys are the process uuid of the nodes. For each uuid key, the
    /// value contains the list of publishers advertising the topic within the
    // same process uuid.
    using MsgAddresses_M = std::map<std::string, std::vector<MessagePublisher>>;

    /// \def SrvAddresses_M
    /// \brief The map stores all the publishers advertising this service.
    /// The keys are the process uuid of the nodes. For each uuid key, the
    /// value contains the list of publishers advertising the service within the
    /// same process uuid.
    using SrvAddresses_M = std::map<std::string, std::vector<ServicePublisher>>;

    /// \def ProtoMsg
    /// \brief An abbreviated protobuf message type.
    using ProtoMsg = google::protobuf::Message;

    /// \def ProtoMsgPtr
    /// \brief Shared pointer to any protobuf message.
    using ProtoMsgPtr = std::shared_ptr<ProtoMsg>;

    /// \def ISubscriptionHandlerPtr
    /// \brief Shared pointer to ISubscriptionHandler.
    using ISubscriptionHandlerPtr = std::shared_ptr<ISubscriptionHandler>;

    /// \def ISubscriptionHandler_M
    /// \brief Map to store the different subscription handlers for a topic.
    /// Each node can have its own subscription handler. The node id
    /// is used as key and a pointer to a generic subscription handler is the
    /// value.
    using ISubscriptionHandler_M =
      std::map<std::string, ISubscriptionHandlerPtr>;

    /// \def IRepHandlerPtr
    /// \brief Shared pointer to IRepHandler.
    using IRepHandlerPtr = std::shared_ptr<IRepHandler>;

    /// \def IReqHandlerPtr
    /// \brief Shared pointer to IReqHandler.
    using IReqHandlerPtr = std::shared_ptr<IReqHandler>;

    /// \def IReqHandler_M
    /// \brief Map to store the different service request handlers for a
    /// topic. Each node can have its own request handler. The node id
    /// is used as key. The value is another map, where the key is the request
    /// UUID and the value is pointer to a generic request handler.
    using IReqHandler_M =
      std::map<std::string, std::map<std::string, IReqHandlerPtr>>;

    /// \def MsgDiscoveryCallback
    /// \brief The user can register callbacks of this type when new connections
    /// or disconnections are detected by the discovery. The prototype of the
    /// callback contains the publisher's information advertising a topic.
    /// E.g.: void onDiscoveryResponse(const MessagePublisher &_publisher).
    using MsgDiscoveryCallback =
      std::function<void(const MessagePublisher&_publisher)>;

    /// \def SrvDiscoveryCallback
    /// \brief The user can register callbacks of this type when new connections
    /// or disconnections are detected by the discovery. The prototype of the
    /// callback contains the publisher's information advertising a service.
    /// E.g.: void onDiscoveryResponse(const ServicePublisher &_publisher).
    using SrvDiscoveryCallback =
      std::function<void(const ServicePublisher&_publisher)>;

    /// \def Timestamp
    /// \brief Used to evaluate the validity of a discovery entry.
    using Timestamp = std::chrono::steady_clock::time_point;
  }
}
#endif
