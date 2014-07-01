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

#include <google/protobuf/message.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ignition
{
  namespace transport
  {
    /// \brief Forward declarations.
    class IRepHandler;
    class IReqHandler;
    class ISubscriptionHandler;
    class NodePrivate;

    /// \def Scope This strongly typed enum defines the different options for
    /// the scope of a topic:
    /// * Process: Topic only available to subscribers in the same process as
    ///            the publisher.
    /// * Host:    Topic only available to subscribers in the same machine as
    ///            the publisher.
    /// * All:     Topic available to any subscriber. This is the default scope.
    enum class Scope {Process, Host, All};

    /// \def The discovery layer can advertise two different types:
    /// * Msg: Regular pub/sub message.
    /// * Srv: Service call.
    enum class MsgType {Msg, Srv};

    /// \def Address_t All the data associated to a topic's publisher.
    struct Address_t
    {
      std::string addr;
      std::string ctrl;
      std::string nUuid;
      Scope scope;
    };

    /// \def Addresses_M
    /// \brief The map stores all the publishers advertising this topic.
    /// The keys are the process uuid of the nodes. For each uuid key, the
    /// value contains the list of {0MQ and 0MQ control addresses} advertising
    /// the topic within the same process uuid.
    typedef std::map<std::string, std::vector<Address_t>> Addresses_M;

    /// \def ProtoMsg
    /// \brief An abbreviated protobuf message type.
    typedef google::protobuf::Message ProtoMsg;

    /// \def ProtoMsgPtr
    /// \brief Shared pointer to any protobuf message.
    typedef std::shared_ptr<ProtoMsg> ProtoMsgPtr;

    /// \def ReqCallback
    /// \brief Callback used for receiving a service call request with the
    /// following parameters:
    /// \param[in] _topic Service name.
    /// \param[in] _req Protobuf message containing the service request.
    /// \param[out] _rep Protobuf message containing the service response.
    /// \return True when the service response is considered successful or
    /// false otherwise.
    typedef std::function<bool (const std::string &_topic,
        const ProtoMsgPtr _req, ProtoMsgPtr _rep)> ReqCallback;

    /// \def RepCallback
    /// \brief Callback used for receiving a service call response with the
    /// following parameters:
    /// \param[in] _topic Service name.
    /// \param[in] _rep Protobuf message containing the service response.
    /// \param[in] _result True when the service call was successful or false
    /// otherwise.
    typedef std::function<void (const std::string &_topic,
      const ProtoMsgPtr _rep, bool _result)> RepCallback;

    /// \def NodePrivatePtr
    /// \brief Pointer to internal class NodePrivate.
    typedef std::unique_ptr<transport::NodePrivate> NodePrivatePtr;

    /// \def ISubscriptionHandlerPtr
    /// \brief Shared pointer to ISubscriptionHandler.
    typedef std::shared_ptr<ISubscriptionHandler> ISubscriptionHandlerPtr;

    /// \def ISubscriptionHandler_M
    /// \brief Map to store the different subscription handlers for a topic.
    /// Each node can have its own subscription handler. The node id
    /// is used as key and a pointer to a generic subscription handler is the
    /// value.
    typedef std::map<std::string, ISubscriptionHandlerPtr>
      ISubscriptionHandler_M;

    /// \def IRepHandlerPtr
    /// \brief Shared pointer to IRepHandler.
    typedef std::shared_ptr<IRepHandler> IRepHandlerPtr;

    /// \def IReqHandlerPtr
    /// \brief Shared pointer to IReqHandler.
    typedef std::shared_ptr<IReqHandler> IReqHandlerPtr;

    /// \def IReqHandler_M
    /// \brief Map to store the different service request handlers for a
    /// topic. Each node can have its own request handler. The node id
    /// is used as key. The value is another map, where the key is the request
    /// UUID and the value is pointer to a generic request handler.
    typedef std::map<std::string, std::map<std::string, IReqHandlerPtr>>
      IReqHandler_M;

    /// \def DiscoveryCallback
    /// \brief The user can register callbacks of this type when new connections
    /// or disconnections are detected by the discovery. The prototype of the
    /// callback is: topic name, 0MQ address, 0MQ control address, UUID of the
    /// process advertising the topic, UUID of the node advertising the topic.
    /// E.g.: void onDiscoveryResponse(const std::string &_topic,
    /// const std::string &_addr, const std::string &_ctrl,
    /// const std::string &_pUuid, const std::string &_nUuid,
    /// const Scope &_scope).
    typedef std::function<void(const std::string &_topic,
      const std::string &_addr, const std::string &_ctrl,
      const std::string &_pUuid, const std::string &_nUuid,
      const Scope &_scope)> DiscoveryCallback;
  }
}
#endif
