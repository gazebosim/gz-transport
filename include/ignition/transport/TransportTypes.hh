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
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ignition
{
  namespace transport
  {
    /// \brief Forward declarations for transport
    class ISubscriptionHandler;
    class NodePrivate;
    class TopicInfo;

    /// \def Scope This strongly typed enum defines the different options for
    /// the scope of a topic:
    /// * Process: Topic only available to subscribers in the same process as
    ///            the publisher.
    /// * Host:    Topic only available to subscribers in the same machine as
    ///            the publisher.
    /// * All:     Topic available to any subscriber. This is the default scope.
    enum class Scope {Process, Host, All};

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
    /// \brief Callback used for receiving a service call request.
    typedef std::function<void (const std::string &, int,
                                const std::string &)> ReqCallback;

    /// \def RepCallback
    /// \brief Callback used for receving a service call response.
    typedef std::function<int (const std::string &,
                               const std::string &,
                               std::string &)> RepCallback;

    /// \def Topics_M
    /// \brief Map used for store all the knowledge about a given topic.
    typedef std::map<std::string, std::shared_ptr<TopicInfo>> Topics_M;

    /// \def NodePrivatePtr
    /// \brief Shared pointer to NodePrivate.
    typedef std::shared_ptr<transport::NodePrivate> NodePrivatePtr;

    /// \def ISubscriptionHandlerPtr
    /// \brief Shared pointer to ISubscriptionHandler.
    typedef std::shared_ptr<ISubscriptionHandler> ISubscriptionHandlerPtr;

    /// \def ISubscriptionHandler_M
    /// \brief Map to store the different subscription handlers for a topic.
    /// Each node can have its own subscription handler. The node id
    /// is used as key.
    typedef std::map<std::string, ISubscriptionHandlerPtr>
        ISubscriptionHandler_M;

    /// \def DiscoveryCallback
    /// \brief The user can register callbacks of this type when new connections
    /// or disconnections are detected by the discovery. The prototype of the
    /// callback is: topic name, 0MQ address, 0MQ control address, UUDI of the
    /// node advertising the topic.
    /// E.g.: void onDiscoveryResponse(const std::string &_topic,
    /// const std::string &_addr, const std::string &_ctrl,
    /// const std::string &_procUuid, const std::string &_nodeUuid,
    /// const Scope &_scope).
    typedef std::function<void(const std::string &, const std::string &,
      const std::string &, const std::string &, const std::string &,
      const Scope &)> DiscoveryCallback;
  }
}
#endif
