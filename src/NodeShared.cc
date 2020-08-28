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

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <zmq.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// TODO(anyone): Remove after fixing the warnings.
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <ignition/msgs.hh>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

#include "NodeSharedPrivate.hh"

#ifdef _MSC_VER
# pragma warning(disable: 4503)
#endif

using namespace std::chrono_literals;
using namespace ignition;
using namespace transport;

const char kIgnAuthDomain[] = "ign-auth";

// Enum that encapsulates the possible values for ZeroMQ's setsocketopt
// for ZMQ_PLAIN_SERVER. A value of 1 enables
// plain authentication server, and a value of 0 disables.
enum class ZmqPlainSecurityServerOptions
{
  // Value to disable plain security server
  ZMQ_PLAIN_SECURITY_SERVER_DISABLED = 0,
  // Value to enable plain security server
  ZMQ_PLAIN_SECURITY_SERVER_ENABLED = 1,
};

//////////////////////////////////////////////////
// Helper to get the username and password
bool userPass(std::string &_user, std::string &_pass)
{
  char *username = std::getenv("IGN_TRANSPORT_USERNAME");
  char *password = std::getenv("IGN_TRANSPORT_PASSWORD");

  if (!username || !password)
    return false;

  _user = username;
  _pass = password;
  return true;
}

//////////////////////////////////////////////////
// Helper to send messages
#ifdef IGN_ZMQ_POST_4_3_1
int sendHelper(zmq::socket_t &_pub, const std::string &_data,
    const zmq::send_flags &_type)
{
  zmq::message_t msg(_data.data(), _data.size());

  auto res = _pub.send(msg, _type);
  if (!res)
    return 0;
  else
    return *res;
}
#endif

int sendHelper(zmq::socket_t &_pub, const std::string &_data, int _type)
{
  zmq::message_t msg(_data.data(), _data.size());

#ifdef IGN_ZMQ_POST_4_3_1
  zmq::send_flags flags = zmq::send_flags::none;
  switch (_type)
  {
    case ZMQ_SNDMORE:
      flags = zmq::send_flags::sndmore;
      break;
    case ZMQ_DONTWAIT:
      flags = zmq::send_flags::dontwait;
      break;
    case 0:
      break;
    default:
      break;
  }
  return sendHelper(_pub, _data, flags);
#else
  return _pub.send(msg, _type);
#endif
}

//////////////////////////////////////////////////
// Helper to receive messages
std::string receiveHelper(zmq::socket_t &_socket)
{
  zmq::message_t msg(0);

#ifdef IGN_ZMQ_POST_4_3_1
  if (!_socket.recv(msg))
#else
  if (!_socket.recv(&msg, 0))
#endif
    return std::string();

  return std::string(reinterpret_cast<char *>(msg.data()), msg.size());
}

//////////////////////////////////////////////////
// Helper to send an authentication error. This is used by basic
// authentication.
void sendAuthErrorHelper(zmq::socket_t &_socket, const std::string &_err)
{
  std::cerr << _err << std::endl;
#ifdef IGN_ZMQ_POST_4_3_1
  sendHelper(_socket, "400", zmq::send_flags::sndmore);
  sendHelper(_socket, _err, zmq::send_flags::sndmore);
  sendHelper(_socket, "", zmq::send_flags::sndmore);
  sendHelper(_socket, "", zmq::send_flags::none);
#else
  sendHelper(_socket, "400", ZMQ_SNDMORE);
  sendHelper(_socket, _err, ZMQ_SNDMORE);
  sendHelper(_socket, "", ZMQ_SNDMORE);
  sendHelper(_socket, "", 0);
#endif
}

//////////////////////////////////////////////////
NodeShared *NodeShared::Instance()
{
#ifdef _MSC_VER
  // If we compile ign-transport as a shared library on Windows, we should
  // never destruct NodeShared, unfortunately. It seems that WinSock does
  // not behave well during the DLL teardown phase as a program exits, and
  // this will confuse the ZeroMQ library into thinking that WinSock
  // misbehaved, causing an assertion in ZeroMQ to fail and throw an exception
  // while the program exits. This is a known issue:
  //
  // https://github.com/zeromq/libzmq/issues/1144
  //
  // An easy way of dodging this issue is to never destruct NodeShared. The
  // Operating System will take care of cleaning up its resources when the
  // application exits. We may want to consider a more elegant solution in
  // the future. The zsys_shutdown() function in the czmq library may be able
  // to provide some inspiration for solving this more cleanly.
  static NodeShared *instance = new NodeShared();
  return instance;
#else
  static NodeShared instance;
  return &instance;
#endif
}

//////////////////////////////////////////////////
NodeShared::NodeShared()
  : verbose(false),
    dataPtr(new NodeSharedPrivate)
{
  // If IGN_VERBOSE=1 enable the verbose mode.
  std::string ignVerbose;
  this->verbose = (env("IGN_VERBOSE", ignVerbose) && ignVerbose == "1");

  // My process UUID.
  Uuid uuid;
  this->pUuid = uuid.ToString();

  // Initialize my discovery services.
  this->dataPtr->msgDiscovery.reset(
      new MsgDiscovery(this->pUuid, this->kMsgDiscPort));
  this->dataPtr->srvDiscovery.reset(
      new SrvDiscovery(this->pUuid, this->kSrvDiscPort));

  // Initialize the 0MQ objects.
  if (!this->InitializeSockets())
    return;

  if (this->verbose)
  {
    std::cout << "Current host address: " << this->hostAddr << std::endl;
    std::cout << "Process UUID: " << this->pUuid << std::endl;
    std::cout << "Bind at: [" << this->myAddress << "] for pub/sub\n";
    std::cout << "Bind at: [" << this->myReplierAddress << "] for srv. calls\n";
    std::cout << "Identity for receiving srv. requests: ["
              << this->replierId.ToString() << "]" << std::endl;
    std::cout << "Identity for receiving srv. responses: ["
              << this->responseReceiverId.ToString() << "]" << std::endl;
  }

  // Start the service thread.
  this->threadReception = std::thread(&NodeShared::RunReceptionTask, this);

  // Set the callback to notify discovery updates (new topics).
  this->dataPtr->msgDiscovery->ConnectionsCb(
      std::bind(&NodeShared::OnNewConnection, this, std::placeholders::_1));

  // Set the callback to notify discovery updates (invalid topics).
  this->dataPtr->msgDiscovery->DisconnectionsCb(
      std::bind(&NodeShared::OnNewDisconnection, this, std::placeholders::_1));

  this->dataPtr->msgDiscovery->RegistrationsCb(
      std::bind(&NodeShared::OnNewRegistration, this, std::placeholders::_1));

  this->dataPtr->msgDiscovery->UnregistrationsCb(
      std::bind(&NodeShared::OnEndRegistration, this, std::placeholders::_1));

  // Set the callback to notify svc discovery updates (new services).
  this->dataPtr->srvDiscovery->ConnectionsCb(
      std::bind(&NodeShared::OnNewSrvConnection, this, std::placeholders::_1));

  // Set the callback to notify svc discovery updates (invalid services).
  this->dataPtr->srvDiscovery->DisconnectionsCb(
      std::bind(&NodeShared::OnNewSrvDisconnection,
        this, std::placeholders::_1));

  // Start the discovery services.
  this->dataPtr->msgDiscovery->Start();
  this->dataPtr->srvDiscovery->Start();

  // Create the local publish thread.
  this->dataPtr->pubThread = std::thread(&NodeSharedPrivate::PublishThread,
      this->dataPtr.get());
}

//////////////////////////////////////////////////
NodeShared::~NodeShared()
{
  // Tell the service thread to terminate.
  this->dataPtr->exit = true;

  // Notify the local pubthread and join.
  this->dataPtr->signalNewPub.notify_all();
  this->dataPtr->pubThread.join();

  // Wait for the service thread before exit.
  if (this->threadReception.joinable())
    this->threadReception.join();

  // Wait for the authentication thread before exit.
  if (this->dataPtr->accessControlThread.joinable())
    this->dataPtr->accessControlThread.join();
}

//////////////////////////////////////////////////
void NodeShared::RunReceptionTask()
{
  while (!this->dataPtr->exit)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {static_cast<void*>(*this->dataPtr->subscriber), 0, ZMQ_POLLIN, 0},
      {static_cast<void*>(*this->dataPtr->replier), 0, ZMQ_POLLIN, 0},
      {static_cast<void*>(*this->dataPtr->responseReceiver), 0, ZMQ_POLLIN, 0}
    };
    try
    {
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]),
                NodeSharedPrivate::Timeout);
    }
    catch(...)
    {
      continue;
    }

    //  If we got a reply, process it.
    if (items[0].revents & ZMQ_POLLIN)
      this->RecvMsgUpdate();
    if (items[1].revents & ZMQ_POLLIN)
      this->RecvSrvRequest();
    if (items[2].revents & ZMQ_POLLIN)
      this->RecvSrvResponse();
  }
}

//////////////////////////////////////////////////
bool NodeShared::Publish(
    const std::string &_topic,
    char *_data,
    const size_t _dataSize, DeallocFunc *_ffn,
    const std::string &_msgType)
{
  try
  {
    // Create the messages.
    // Note that we use zero copy for passing the message data (msg2).
    zmq::message_t msg0(_topic.data(), _topic.size()),
                   msg1(this->myAddress.data(), this->myAddress.size()),
                   msg2(_data, _dataSize, _ffn, nullptr),
                   msg3(_msgType.data(), _msgType.size());

    // Send the messages
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

#ifdef IGN_ZMQ_POST_4_3_1
    this->dataPtr->publisher->send(msg0, zmq::send_flags::sndmore);
    this->dataPtr->publisher->send(msg1, zmq::send_flags::sndmore);
    this->dataPtr->publisher->send(msg2, zmq::send_flags::sndmore);
    this->dataPtr->publisher->send(msg3, zmq::send_flags::none);
#else
    this->dataPtr->publisher->send(msg0, ZMQ_SNDMORE);
    this->dataPtr->publisher->send(msg1, ZMQ_SNDMORE);
    this->dataPtr->publisher->send(msg2, ZMQ_SNDMORE);
    this->dataPtr->publisher->send(msg3, 0);
#endif
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "NodeShared::Publish() Error: " << ze.what() << std::endl;
     return false;
  }

  return true;
}

//////////////////////////////////////////////////
void NodeShared::RecvMsgUpdate()
{
  zmq::message_t msg(0);
  std::string topic;
  // std::string sender;
  std::string data;
  std::string msgType;
  HandlerInfo handlerInfo;

  {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    try
    {
#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->subscriber->recv(msg))
#else
      if (!this->dataPtr->subscriber->recv(&msg, 0))
#endif
        return;
      topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      // TODO(caguero): Use this as extra metadata for the subscriber.
#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->subscriber->recv(msg))
#else
      if (!this->dataPtr->subscriber->recv(&msg, 0))
#endif
        return;
      // sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->subscriber->recv(msg))
#else
      if (!this->dataPtr->subscriber->recv(&msg, 0))
#endif
        return;
      data = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->subscriber->recv(msg))
#else
      if (!this->dataPtr->subscriber->recv(&msg, 0))
#endif
        return;
      msgType = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "Error: " << _error.what() << std::endl;
      return;
    }

    handlerInfo = this->CheckHandlerInfo(topic);
  }

  MessageInfo info;
  info.SetTopicAndPartition(topic);
  info.SetType(msgType);
  this->TriggerCallbacks(info, data, handlerInfo);
}

//////////////////////////////////////////////////
NodeShared::HandlerInfo NodeShared::CheckHandlerInfo(
    const std::string &_topic) const
{
  HandlerInfo info;

  std::lock_guard<std::recursive_mutex> lk(this->mutex);

  info.haveLocal = this->localSubscribers.normal.Handlers(
        _topic, info.localHandlers);

  info.haveRaw = this->localSubscribers.raw.Handlers(
        _topic, info.rawHandlers);

  return info;
}

//////////////////////////////////////////////////
NodeShared::SubscriberInfo NodeShared::CheckSubscriberInfo(
    const std::string &_topic,
    const std::string &_msgType) const
{
  SubscriberInfo info;

  std::lock_guard<std::recursive_mutex> lk(this->mutex);

  info.haveLocal = this->localSubscribers.normal.Handlers(
        _topic, info.localHandlers);

  info.haveRaw = this->localSubscribers.raw.Handlers(
        _topic, info.rawHandlers);

  info.haveRemote = this->remoteSubscribers.HasTopic(
        _topic, _msgType);

  return info;
}

//////////////////////////////////////////////////
void NodeShared::TriggerSubscriberCallbacks(
    const std::string &_topic,
    const std::string &_msgData,
    const std::string &_msgType,
    const HandlerInfo &_handlerInfo)
{
  MessageInfo info;
  info.SetTopicAndPartition(_topic);
  info.SetType(_msgType);

  this->TriggerCallbacks(info, _msgData, _handlerInfo);
}

//////////////////////////////////////////////////
void NodeShared::TriggerCallbacks(
    const MessageInfo &_info,
    const std::string &_msgData,
    const HandlerInfo &_handlerInfo)
{
  if (!_handlerInfo.haveLocal && !_handlerInfo.haveRaw)
    return;

  if (_handlerInfo.haveRaw)
  {
    for (const auto &node : _handlerInfo.rawHandlers)
    {
      for (const auto &handler : node.second)
      {
        const RawSubscriptionHandlerPtr &rawHandler = handler.second;
        if (rawHandler)
        {
          if (rawHandler->TypeName() == _info.Type() ||
              rawHandler->TypeName() == kGenericMessageType)
          {
            rawHandler->RunRawCallback(_msgData.c_str(), _msgData.size(),
                _info);
          }
        }
        else
          std::cerr << "Raw subscription handler is NULL" << std::endl;
      }
    }
  }

  if (_handlerInfo.haveLocal)
  {
    // This will be instantiated by the first suitable handler that we
    // encounter. If there is no suitable handler, then we can avoid
    // deserializing the message altogether.
    std::shared_ptr<ProtoMsg> msg;

    for (const auto &node : _handlerInfo.localHandlers)
    {
      for (const auto &handler : node.second)
      {
        const ISubscriptionHandlerPtr &localHandler = handler.second;
        if (localHandler)
        {
          if (localHandler->TypeName() == _info.Type() ||
              localHandler->TypeName() == kGenericMessageType)
          {
            if (!msg)
            {
              // If the message has not been deserialized yet, do it now since
              // we have allegedly found a subscriber which should be able to
              // do it.
              msg = localHandler->CreateMsg(_msgData, _info.Type());

              if (!msg)
              {
                // If the message could not be created, then none of the
                // handlers in this process will be able to create it, because
                // protobuf has access to all message types that the current
                // process is linked to. If CreateMsg(~,~) fails, then we may
                // as well quit.
                return;
              }
            }

            localHandler->RunLocalCallback(*msg, _info);
          }
        }
        else
          std::cerr << "Local subscription handler is NULL" << std::endl;
      }
    }
  }
}

//////////////////////////////////////////////////
void NodeShared::RecvControlUpdate()
{
}

//////////////////////////////////////////////////
void NodeShared::RecvSrvRequest()
{
  if (verbose)
    std::cout << "Message received requesting a service call" << std::endl;

  zmq::message_t msg(0);
  std::string topic;
  std::string sender;
  std::string nodeUuid;
  std::string reqUuid;
  std::string req;
  std::string rep;
  std::string resultStr;
  std::string dstId;
  std::string reqType;
  std::string repType;

  IRepHandlerPtr repHandler;
  bool hasHandler;

  {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    try
    {
#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      dstId = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      nodeUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      reqUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      req = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      reqType = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->replier->recv(msg))
#else
      if (!this->dataPtr->replier->recv(&msg, 0))
#endif
        return;
      repType = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "NodeShared::RecvSrvRequest() error parsing request: "
                << _error.what() << std::endl;
      return;
    }

    hasHandler =
      this->repliers.FirstHandler(topic, reqType, repType, repHandler);
  }

  // Get the REP handler.
  if (hasHandler)
  {
    // Run the service call and get the results.
    bool result = repHandler->RunCallback(req, rep);

    // If 'reptype' is msgs::Empty", this is a oneway request
    // and we don't send response
    if (repType == ignition::msgs::Empty().GetTypeName())
    {
      return;
    }

    if (result)
      resultStr = "1";
    else
      resultStr = "0";

    {
      std::lock_guard<std::recursive_mutex> lock(this->mutex);
      // I am still not connected to this address.
      if (std::find(this->srvConnections.begin(), this->srvConnections.end(),
            sender) == this->srvConnections.end())
      {
        this->dataPtr->replier->connect(sender.c_str());
        this->srvConnections.push_back(sender);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (this->verbose)
        {
          std::cout << "\t* Connected to [" << sender
                    << "] for sending a response" << std::endl;
        }
      }
    }

    // Send the reply.
    try
    {
      std::lock_guard<std::recursive_mutex> lock(this->mutex);
      zmq::message_t response;

      response.rebuild(dstId.size());
      memcpy(response.data(), dstId.data(), dstId.size());
#ifdef IGN_ZMQ_POST_4_3_1
      this->dataPtr->replier->send(response, zmq::send_flags::sndmore);
#else
      this->dataPtr->replier->send(response, ZMQ_SNDMORE);
#endif

      response.rebuild(topic.size());
      memcpy(response.data(), topic.data(), topic.size());
#ifdef IGN_ZMQ_POST_4_3_1
      this->dataPtr->replier->send(response, zmq::send_flags::sndmore);
#else
      this->dataPtr->replier->send(response, ZMQ_SNDMORE);
#endif

      response.rebuild(nodeUuid.size());
      memcpy(response.data(), nodeUuid.data(), nodeUuid.size());
#ifdef IGN_ZMQ_POST_4_3_1
      this->dataPtr->replier->send(response, zmq::send_flags::sndmore);
#else
      this->dataPtr->replier->send(response, ZMQ_SNDMORE);
#endif

      response.rebuild(reqUuid.size());
      memcpy(response.data(), reqUuid.data(), reqUuid.size());
#ifdef IGN_ZMQ_POST_4_3_1
      this->dataPtr->replier->send(response, zmq::send_flags::sndmore);
#else
      this->dataPtr->replier->send(response, ZMQ_SNDMORE);
#endif

      response.rebuild(rep.size());
      memcpy(response.data(), rep.data(), rep.size());
#ifdef IGN_ZMQ_POST_4_3_1
      this->dataPtr->replier->send(response, zmq::send_flags::sndmore);
#else
      this->dataPtr->replier->send(response, ZMQ_SNDMORE);
#endif

      response.rebuild(resultStr.size());
      memcpy(response.data(), resultStr.data(), resultStr.size());
#ifdef IGN_ZMQ_POST_4_3_1
      this->dataPtr->replier->send(response, zmq::send_flags::none);
#else
      this->dataPtr->replier->send(response, 0);
#endif
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "NodeShared::RecvSrvRequest() error sending response: "
                << _error.what() << std::endl;
      return;
    }
  }
  // else
  //   std::cerr << "I do not have a service call registered for topic ["
  //             << topic << "]\n";
}

//////////////////////////////////////////////////
void NodeShared::RecvSrvResponse()
{
  if (verbose)
    std::cout << "Message received containing a service call REP" << std::endl;

  zmq::message_t msg(0);
  std::string topic;
  std::string nodeUuid;
  std::string reqUuid;
  std::string rep;
  std::string resultStr;
  bool result;

  IReqHandlerPtr reqHandlerPtr;
  bool hasHandler;

  {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    try
    {
#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->responseReceiver->recv(msg))
#else
      if (!this->dataPtr->responseReceiver->recv(&msg, 0))
#endif
        return;

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->responseReceiver->recv(msg))
#else
      if (!this->dataPtr->responseReceiver->recv(&msg, 0))
#endif
        return;
      topic = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->responseReceiver->recv(msg))
#else
      if (!this->dataPtr->responseReceiver->recv(&msg, 0))
#endif
        return;
      nodeUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->responseReceiver->recv(msg))
#else
      if (!this->dataPtr->responseReceiver->recv(&msg, 0))
#endif
        return;
      reqUuid = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->responseReceiver->recv(msg))
#else
      if (!this->dataPtr->responseReceiver->recv(&msg, 0))
#endif
        return;
      rep = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

#ifdef IGN_ZMQ_POST_4_3_1
      if (!this->dataPtr->responseReceiver->recv(msg))
#else
      if (!this->dataPtr->responseReceiver->recv(&msg, 0))
#endif
        return;
      resultStr = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
      result = resultStr == "1";
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "NodeShared::RecvSrvResponse() error: "
                << _error.what() << std::endl;
      return;
    }

    hasHandler =
      this->requests.Handler(topic, nodeUuid, reqUuid, reqHandlerPtr);
  }

  if (hasHandler)
  {
    // Notify the result.
    reqHandlerPtr->NotifyResult(rep, result);

    // Remove the handler.
    std::lock_guard<std::recursive_mutex> lock(this->mutex);
    {
      if (!this->requests.RemoveHandler(topic, nodeUuid, reqUuid))
      {
        std::cerr << "NodeShare::RecvSrvResponse(): "
                  << "Error removing request handler" << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "Received a service call response but I don't have a handler"
              << " for it" << std::endl;
  }
}

//////////////////////////////////////////////////
void NodeShared::SendPendingRemoteReqs(const std::string &_topic,
  const std::string &_reqType, const std::string &_repType)
{
  std::string responserAddr;
  std::string responserId;
  SrvAddresses_M addresses;
  this->dataPtr->srvDiscovery->Publishers(_topic, addresses);
  if (addresses.empty())
    return;

  // Find a publisher that offers this service with a particular pair of REQ/REP
  // types.
  bool found = false;
  for (auto &proc : addresses)
  {
    auto &v = proc.second;
    for (auto &pub : v)
    {
      if (pub.ReqTypeName() == _reqType && pub.RepTypeName() == _repType)
      {
        found = true;
        responserAddr = pub.Addr();
        responserId = pub.SocketId();
        break;
      }
    }
    if (found)
      break;
  }

  if (!found)
    return;

  if (verbose)
  {
    std::cout << "Found a service call responser at ["
              << responserAddr << "]" << std::endl;
  }

  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  // I am still not connected to this address.
  if (std::find(this->srvConnections.begin(), this->srvConnections.end(),
        responserAddr) == this->srvConnections.end())
  {
    this->dataPtr->requester->connect(responserAddr.c_str());
    this->srvConnections.push_back(responserAddr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (this->verbose)
    {
      std::cout << "\t* Connected to [" << responserAddr
                << "] for service requests" << std::endl;
    }
  }

  // Send all the pending REQs.
  IReqHandler_M reqs;
  if (!this->requests.Handlers(_topic, reqs))
    return;

  for (auto &node : reqs)
  {
    for (auto &req : node.second)
    {
      // Check if this service call has been already requested.
      if (req.second->Requested())
        continue;

      // Check that the pending service call has types that match the responser.
      if (req.second->ReqTypeName() != _reqType ||
          req.second->RepTypeName() != _repType)
      {
        continue;
      }

      // Mark the handler as requested.
      req.second->Requested(true);

      std::string data;
      if (!req.second->Serialize(data))
        continue;

      auto nodeUuid = req.second->NodeUuid();
      auto reqUuid = req.second->HandlerUuid();

      try
      {
        zmq::message_t msg;

        msg.rebuild(responserId.size());
        memcpy(msg.data(), responserId.data(), responserId.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(_topic.size());
        memcpy(msg.data(), _topic.data(), _topic.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(this->myRequesterAddress.size());
        memcpy(msg.data(), this->myRequesterAddress.data(),
          this->myRequesterAddress.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        std::string myId = this->responseReceiverId.ToString();
        msg.rebuild(myId.size());
        memcpy(msg.data(), myId.data(), myId.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(nodeUuid.size());
        memcpy(msg.data(), nodeUuid.data(), nodeUuid.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(reqUuid.size());
        memcpy(msg.data(), reqUuid.data(), reqUuid.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(data.size());
        memcpy(msg.data(), data.data(), data.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(_reqType.size());
        memcpy(msg.data(), _reqType.data(), _reqType.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::sndmore);
#else
        this->dataPtr->requester->send(msg, ZMQ_SNDMORE);
#endif

        msg.rebuild(_repType.size());
        memcpy(msg.data(), _repType.data(), _repType.size());
#ifdef IGN_ZMQ_POST_4_3_1
        this->dataPtr->requester->send(msg, zmq::send_flags::none);
#else
        this->dataPtr->requester->send(msg, 0);
#endif
      }
      catch(const zmq::error_t& /*ze*/)
      {
        // Debug output.
        // std::cerr << "Error connecting [" << ze.what() << "]\n";
      }

      // Remove the handler associated to this service request. We won't
      // receive a response because this is a oneway request.
      if (_repType == ignition::msgs::Empty().GetTypeName())
      {
        this->requests.RemoveHandler(_topic, nodeUuid, reqUuid);
      }
    }
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewConnection(const MessagePublisher &_pub)
{
  std::string topic = _pub.Topic();
  std::string addr = _pub.Addr();
  std::string procUuid = _pub.PUuid();

  if (this->verbose)
  {
    std::cout << "Connection callback" << std::endl;
    std::cout << _pub;
  }

  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  // Check if we are interested in this topic.
  if (this->localSubscribers.HasSubscriber(topic) &&
      this->pUuid.compare(procUuid) != 0)
  {
    // Handle security
    this->dataPtr->SecurityOnNewConnection();

    // I am not connected to the process.
    if (!this->connections.HasPublisher(addr))
      this->dataPtr->subscriber->connect(addr.c_str());

    // Add a new filter for the topic.
#ifdef IGN_CPPZMQ_POST_4_7_0
    this->dataPtr->subscriber->set(zmq::sockopt::subscribe, topic);
#else
    this->dataPtr->subscriber->setsockopt(ZMQ_SUBSCRIBE,
        topic.data(), topic.size());
#endif

    // Register the new connection with the publisher.
    this->connections.AddPublisher(_pub);

    if (this->verbose)
      std::cout << "\t* Connected to [" << addr << "] for data\n";

    MessagePublisher pub(_pub);
    pub.SetPUuid(this->pUuid);

    // Hack: We use this field to store the PUuid of the topic publisher.
    pub.SetCtrl(_pub.PUuid());

    std::vector<std::string> handlerNodeUuids =
        this->localSubscribers.NodeUuids(topic, _pub.MsgTypeName());
    for (const std::string &nodeUuid : handlerNodeUuids)
    {
      pub.SetNUuid(nodeUuid);

      // Send a message to the publisher notify it
      // about all my remoteSubscribers.
      this->dataPtr->msgDiscovery->Register(pub);
    }
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewDisconnection(const MessagePublisher &_pub)
{
  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  std::string topic = _pub.Topic();
  std::string procUuid = _pub.PUuid();
  std::string nUuid = _pub.NUuid();

  if (this->verbose)
  {
    std::cout << "New disconnection detected " << std::endl;
    std::cout << "\tProcess UUID: " << procUuid << std::endl;
  }

  // A remote subscriber[s] has been disconnected.
  if (topic != "" && nUuid != "")
  {
    this->remoteSubscribers.DelPublisherByNode(topic, procUuid, nUuid);

    MessagePublisher connection;
    if (!this->connections.Publisher(topic, procUuid, nUuid, connection))
      return;

    // I am no longer connected.
    this->connections.DelPublisherByNode(topic, procUuid, nUuid);
  }
  else
  {
    // Note: We deliberately don't remove the list of remote subscribers
    // for this process. Remote nodes might suffer package delays (due to WiFi
    // or traffic load) and if we remove them, they won't be able to receive
    // data anymore.

    MsgAddresses_M info;
    if (!this->connections.Publishers(topic, info))
      return;

    // Remove all the connections from the process disonnected.
    this->connections.DelPublishersByProc(procUuid);
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewSrvConnection(const ServicePublisher &_pub)
{
  std::string topic = _pub.Topic();
  std::string addr = _pub.Addr();
  std::string reqType = _pub.ReqTypeName();
  std::string repType = _pub.RepTypeName();

  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  if (this->verbose)
  {
    std::cout << "Service call connection callback" << std::endl;
    std::cout << _pub;
  }

  // I am still not connected to this address.
  if (std::find(this->srvConnections.begin(), this->srvConnections.end(),
        addr) == this->srvConnections.end())
  {
    this->dataPtr->requester->connect(addr.c_str());
    this->srvConnections.push_back(addr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (this->verbose)
    {
      std::cout << "\t* Connected to [" << addr
                << "] for service requests" << std::endl;
    }
  }

  // Check if there's a pending service request with this specific combination
  // of request and response types.
  IReqHandlerPtr handler;
  if (this->requests.FirstHandler(topic, reqType, repType, handler))
  {
    // Request all pending service calls for this topic and req/rep types.
    this->SendPendingRemoteReqs(topic, reqType, repType);
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewSrvDisconnection(const ServicePublisher &_pub)
{
  std::string addr = _pub.Addr();

  std::lock_guard<std::recursive_mutex> lock(this->mutex);

  // Remove the address from the list of connected addresses.
  this->srvConnections.erase(std::remove(std::begin(this->srvConnections),
    std::end(this->srvConnections), addr.c_str()),
    std::end(this->srvConnections));

  if (this->verbose)
  {
    std::cout << "Service call disconnection callback" << std::endl;
    std::cout << _pub;
  }
}

//////////////////////////////////////////////////
void NodeShared::OnNewRegistration(const MessagePublisher &_pub)
{
  // Discard the message if the destination PUUID is not me.
  if (_pub.Ctrl() != this->pUuid)
    return;

  std::string procUuid = _pub.PUuid();
  std::string nodeUuid = _pub.NUuid();

  if (this->verbose)
  {
    std::cout << "Registering a new remote connection" << std::endl;
    std::cout << "\tProc UUID: [" << procUuid << "]" << std::endl;
    std::cout << "\tNode UUID: [" << nodeUuid << "]" << std::endl;
  }

  // Add a remote subscriber.
  std::lock_guard<std::recursive_mutex> lock(this->mutex);
  this->remoteSubscribers.AddPublisher(_pub);
}

//////////////////////////////////////////////////
void NodeShared::OnEndRegistration(const MessagePublisher &_pub)
{
  // Discard the message if the destination PUUID is not me.
  if (_pub.Ctrl() != this->pUuid)
    return;

  std::string topic = _pub.Topic();
  std::string procUuid = _pub.PUuid();
  std::string nodeUuid = _pub.NUuid();

  if (this->verbose)
  {
    std::cout << "Registering the end of a remote connection" << std::endl;
    std::cout << "\tProc UUID: " << procUuid << std::endl;
    std::cout << "\tNode UUID: [" << nodeUuid << "]" << std::endl;
  }

  // Delete a remote subscriber.
  std::lock_guard<std::recursive_mutex> lock(this->mutex);
  this->remoteSubscribers.DelPublisherByNode(topic, procUuid, nodeUuid);
}

//////////////////////////////////////////////////
bool NodeShared::InitializeSockets()
{
  try
  {
    // Set the hostname's ip address.
    this->hostAddr = this->dataPtr->msgDiscovery->HostAddr();

    // Publisher socket listening in a random port.
    std::string anyTcpEp = "tcp://" + this->hostAddr + ":*";

    // Initialize security
    this->dataPtr->SecurityInit();


    int lingerVal = 0;
#ifdef IGN_CPPZMQ_POST_4_7_0
    this->dataPtr->publisher->set(zmq::sockopt::linger, lingerVal);
#else
    this->dataPtr->publisher->setsockopt(ZMQ_LINGER,
        &lingerVal, sizeof(lingerVal));
#endif

    // Set the capacity of the buffer for receiving messages.
    std::string ignRcvHwm;
    int rcvQueueVal = kDefaultRcvHwm;
    if (env("IGN_TRANSPORT_RCVHWM", ignRcvHwm))
    {
      try
      {
        rcvQueueVal = std::stoi(ignRcvHwm);
      }
      catch (std::invalid_argument &_e)
      {
        std::cerr << "Unable to convert IGN_TRANSPORT_RCVHWM value ["
                  << ignRcvHwm << "] to an integer number. Using ["
                  << rcvQueueVal << "] instead." << std::endl;
      }
      catch (std::out_of_range &_e)
      {
        std::cerr << "Unable to convert IGN_TRANSPORT_RCVHWM value ["
                  << ignRcvHwm << "] to an integer number. This number is "
                  << "out of range. Using [" << rcvQueueVal << "] instead."
                  << std::endl;
      }
      if (rcvQueueVal < 0)
      {
        rcvQueueVal = kDefaultRcvHwm;
        std::cerr << "Unable to convert IGN_TRANSPORT_RCVHWM value ["
                  << ignRcvHwm << "] to a non-negative number. This number is "
                  << "negative. Using [" << rcvQueueVal << "] instead."
                  << std::endl;
      }
    }
#ifdef IGN_CPPZMQ_POST_4_7_0
    this->dataPtr->subscriber->set(zmq::sockopt::rcvhwm, rcvQueueVal);
#else
    this->dataPtr->subscriber->setsockopt(ZMQ_RCVHWM,
          &rcvQueueVal, sizeof(rcvQueueVal));
#endif

    // Set the capacity of the buffer for sending messages.
    std::string ignSndHwm;
    int sndQueueVal = kDefaultSndHwm;
    if (env("IGN_TRANSPORT_SNDHWM", ignSndHwm))
    {
      try
      {
        sndQueueVal = std::stoi(ignSndHwm);
      }
      catch (std::invalid_argument &_e)
      {
        std::cerr << "Unable to convert IGN_TRANSPORT_SNDHWM value ["
                  << ignSndHwm << "] to an integer number. Using ["
                  << sndQueueVal << "] instead." << std::endl;
      }
      catch (std::out_of_range &_e)
      {
        std::cerr << "Unable to convert IGN_TRANSPORT_SNDHWM value ["
                  << ignSndHwm << "] to an integer number. This number is "
                  << "out of range. Using [" << sndQueueVal << "] instead."
                  << std::endl;
      }
      if (sndQueueVal < 0)
      {
        sndQueueVal = kDefaultSndHwm;
        std::cerr << "Unable to convert IGN_TRANSPORT_SNDHWM value ["
                  << ignSndHwm << "] to a non-negative number. This number is "
                  << "negative. Using [" << sndQueueVal << "] instead."
                  << std::endl;
      }
    }
#ifdef IGN_CPPZMQ_POST_4_7_0
    this->dataPtr->publisher->set(zmq::sockopt::sndhwm, sndQueueVal);

    this->dataPtr->publisher->bind(anyTcpEp.c_str());
    this->myAddress =
        this->dataPtr->publisher->get(zmq::sockopt::last_endpoint);

    // ResponseReceiver socket listening in a random port.
    std::string id = this->responseReceiverId.ToString();
    this->dataPtr->responseReceiver->set(zmq::sockopt::routing_id, id);
    this->dataPtr->responseReceiver->bind(anyTcpEp.c_str());
    this->myRequesterAddress = this->dataPtr->responseReceiver->get(
        zmq::sockopt::last_endpoint);

    // Replier socket listening in a random port.
    id = this->replierId.ToString();
    this->dataPtr->replier->set(zmq::sockopt::routing_id, id);
    int routeOn = 1;
    this->dataPtr->replier->set(zmq::sockopt::linger, lingerVal);
    this->dataPtr->replier->set(zmq::sockopt::router_mandatory, routeOn);
    this->dataPtr->replier->bind(anyTcpEp.c_str());
    this->myReplierAddress =
        this->dataPtr->replier->get(zmq::sockopt::last_endpoint);

    this->dataPtr->requester->set(zmq::sockopt::linger, lingerVal);
    this->dataPtr->requester->set(zmq::sockopt::router_mandatory, routeOn);
#else
    char bindEndPoint[1024];
    this->dataPtr->publisher->setsockopt(ZMQ_SNDHWM,
        &sndQueueVal, sizeof(sndQueueVal));

    this->dataPtr->publisher->bind(anyTcpEp.c_str());
    size_t size = sizeof(bindEndPoint);
    this->dataPtr->publisher->getsockopt(ZMQ_LAST_ENDPOINT,
        &bindEndPoint, &size);
    this->myAddress = bindEndPoint;

    // ResponseReceiver socket listening in a random port.
    std::string id = this->responseReceiverId.ToString();
    this->dataPtr->responseReceiver->setsockopt(ZMQ_IDENTITY,
        id.c_str(), id.size());
    this->dataPtr->responseReceiver->bind(anyTcpEp.c_str());
    this->dataPtr->responseReceiver->getsockopt(ZMQ_LAST_ENDPOINT,
        &bindEndPoint, &size);
    this->myRequesterAddress = bindEndPoint;

    // Replier socket listening in a random port.
    id = this->replierId.ToString();
    this->dataPtr->replier->setsockopt(ZMQ_IDENTITY, id.c_str(), id.size());
    int RouteOn = 1;
    this->dataPtr->replier->setsockopt(ZMQ_LINGER,
        &lingerVal, sizeof(lingerVal));
    this->dataPtr->replier->setsockopt(ZMQ_ROUTER_MANDATORY,
        &RouteOn, sizeof(RouteOn));
    this->dataPtr->replier->bind(anyTcpEp.c_str());
    this->dataPtr->replier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myReplierAddress = bindEndPoint;

    this->dataPtr->requester->setsockopt(ZMQ_LINGER,
        &lingerVal, sizeof(lingerVal));
    this->dataPtr->requester->setsockopt(ZMQ_ROUTER_MANDATORY, &RouteOn,
      sizeof(RouteOn));
#endif
  }
  catch(const zmq::error_t& ze)
  {
    std::cerr << "InitializeSockets() Error: " << ze.what() << std::endl;
    std::cerr << "Ignition Transport has not been correctly initialized"
              << std::endl;
    return false;
  }

  return true;
}

/////////////////////////////////////////////////
bool NodeShared::TopicPublishers(const std::string &_topic,
                                 SrvAddresses_M &_publishers) const
{
  return this->dataPtr->srvDiscovery->Publishers(_topic, _publishers);
}

/////////////////////////////////////////////////
bool NodeShared::DiscoverService(const std::string &_topic) const
{
  return this->dataPtr->srvDiscovery->Discover(_topic);
}

/////////////////////////////////////////////////
bool NodeShared::AdvertisePublisher(const ServicePublisher &_publisher)
{
  return this->dataPtr->srvDiscovery->Advertise(_publisher);
}

/////////////////////////////////////////////////
int NodeShared::RcvHwm()
{
  int rcvHwm;
  try
  {
#ifdef IGN_CPPZMQ_POST_4_7_0
    rcvHwm = this->dataPtr->subscriber->get(zmq::sockopt::rcvhwm);
#else
    size_t rcvHwmSize = sizeof(rcvHwm);
    this->dataPtr->subscriber->getsockopt(ZMQ_RCVHWM, &rcvHwm, &rcvHwmSize);
#endif
  }
  catch (zmq::error_t &_e)
  {
    return -1;
  }

  return rcvHwm;
}

/////////////////////////////////////////////////
int NodeShared::SndHwm()
{
  int sndHwm;
  try
  {
#ifdef IGN_CPPZMQ_POST_4_7_0
    sndHwm = this->dataPtr->publisher->get(zmq::sockopt::sndhwm);
#else
    size_t sndHwmSize = sizeof(sndHwm);
    this->dataPtr->publisher->getsockopt(ZMQ_SNDHWM, &sndHwm, &sndHwmSize);
#endif
  }
  catch (zmq::error_t &_e)
  {
    return -1;
  }

  return sndHwm;
}

//////////////////////////////////////////////////
bool NodeShared::HandlerWrapper::HasSubscriber(
    const std::string &_fullyQualifiedTopic,
    const std::string &_msgType) const
{
  std::shared_ptr<ISubscriptionHandler> normalSubscriberPtr;
  std::shared_ptr<RawSubscriptionHandler> rawSubscriberPtr;

  return this->normal.FirstHandler(
            _fullyQualifiedTopic, _msgType, normalSubscriberPtr)
         || this->raw.FirstHandler(
            _fullyQualifiedTopic, _msgType, rawSubscriberPtr);
}

//////////////////////////////////////////////////
bool NodeShared::HandlerWrapper::HasSubscriber(
    const std::string &_fullyQualifiedTopic) const
{
  return this->normal.HasHandlersForTopic(_fullyQualifiedTopic)
      || this->raw.HasHandlersForTopic(_fullyQualifiedTopic);
}

//////////////////////////////////////////////////
template <typename HandlerT>
static void AppendNodeUuids(const HandlerStorage<HandlerT> &_handlerStorage,
                            const std::string &_fullyQualifiedTopic,
                            const std::string &_msgTypeName,
                            std::vector<std::string> &_uuids)
{
  using HandlerTPtr = std::shared_ptr<HandlerT>;
  std::map<std::string, std::map<std::string, HandlerTPtr>> handlers;

  _handlerStorage.Handlers(_fullyQualifiedTopic, handlers);
  for (const auto &collection : handlers)
  {
    for (const auto &collectionEntry : collection.second)
    {
      const HandlerTPtr &handler = collectionEntry.second;
      const std::string &handlerMsgType = handler->TypeName();
      if (handlerMsgType == _msgTypeName
          || handlerMsgType == kGenericMessageType)
      {
        _uuids.push_back(handler->NodeUuid());
      }
    }
  }
}

//////////////////////////////////////////////////
std::vector<std::string> NodeShared::HandlerWrapper::NodeUuids(
    const std::string &_fullyQualifiedTopic,
    const std::string &_msgTypeName) const
{
  std::vector<std::string> uuids;
  AppendNodeUuids(this->normal, _fullyQualifiedTopic, _msgTypeName, uuids);
  AppendNodeUuids(this->raw, _fullyQualifiedTopic, _msgTypeName, uuids);

  return uuids;
}

//////////////////////////////////////////////////
bool NodeShared::HandlerWrapper::RemoveHandlersForNode(
    const std::string &_fullyQualifiedTopic,
    const std::string &_nUuid)
{
  bool removed = false;
  removed |= this->normal.RemoveHandlersForNode(_fullyQualifiedTopic, _nUuid);
  removed |= this->raw.RemoveHandlersForNode(_fullyQualifiedTopic, _nUuid);

  return removed;
}


//////////////////////////////////////////////////
void NodeSharedPrivate::SecurityOnNewConnection()
{
  std::string user, pass;

  // Set username and pass if they exist
  // \todo(anyone): This will cause the subscriber to connect only to secure
  // connections. Would be nice if the subscriber could still connect to
  // unsecure connections. This might require an unsecure and secure
  // subscriber.
  // See issue #74
  if (userPass(user, pass))
  {
#ifdef IGN_CPPZMQ_POST_4_7_0
    this->subscriber->set(zmq::sockopt::plain_username, user);
    this->subscriber->set(zmq::sockopt::plain_password, pass);
#else
    this->subscriber->setsockopt(ZMQ_PLAIN_USERNAME, user.c_str(), user.size());
    this->subscriber->setsockopt(ZMQ_PLAIN_PASSWORD, pass.c_str(), pass.size());
#endif
  }
}

//////////////////////////////////////////////////
void NodeSharedPrivate::SecurityInit()
{
  // Check if a username and password has been set. If so, then
  // setup a PLAIN authentication server.
  std::string user, pass;
  if (userPass(user, pass))
  {
    // Create the access control thread.
    this->accessControlThread = std::thread(
        &NodeSharedPrivate::AccessControlHandler, this);

    int asPlainSecurityServer = static_cast<int>(
        ZmqPlainSecurityServerOptions::ZMQ_PLAIN_SECURITY_SERVER_ENABLED);

#ifdef IGN_CPPZMQ_POST_4_7_0
    this->publisher->set(zmq::sockopt::plain_server, asPlainSecurityServer);
    this->publisher->set(zmq::sockopt::zap_domain, kIgnAuthDomain);
#else
    this->publisher->setsockopt(ZMQ_PLAIN_SERVER,
        &asPlainSecurityServer, sizeof(asPlainSecurityServer));
    this->publisher->setsockopt(ZMQ_ZAP_DOMAIN, kIgnAuthDomain,
        std::strlen(kIgnAuthDomain));
#endif
  }
}

//////////////////////////////////////////////////
// Access control handler for plain security.
// This function is designed to be run in a thread.
void NodeSharedPrivate::AccessControlHandler()
{
  zmq::socket_t *sock = new zmq::socket_t(*this->context, ZMQ_REP);

  try
  {
    // Bind to the zap address
    sock->bind("inproc://zeromq.zap.01");

    // Get the username and password
    std::string user, pass;
    if (!userPass(user, pass))
    {
      std::cerr << "Username and password not set. "
                << "Authentication is disabled\n";
      return;
    }

    std::string sequence;
    std::string domain;
    std::string address;
    std::string routingId;
    std::string mechanism;
    std::string givenUsername;
    std::string givenPassword;
    std::string version;

    zmq::pollitem_t items[] =
    {
      {static_cast<void*>(*sock), 0, ZMQ_POLLIN, 0},
    };

    // Process
    while (!this->exit)
    {
      try
      {
        zmq::poll(&items[0], sizeof(items) / sizeof(items[0]),
            NodeSharedPrivate::Timeout);
      }
      catch(...)
      {
        continue;
      }

      if (items[0].revents & ZMQ_POLLIN)
      {
        // Get the version.
        version = receiveHelper(*sock);
        if (version.empty())
          break;

        // Get remaining data
        sequence = receiveHelper(*sock);
        domain = receiveHelper(*sock);
        address = receiveHelper(*sock);
        routingId = receiveHelper(*sock);
        mechanism = receiveHelper(*sock);
        givenUsername = receiveHelper(*sock);
        givenPassword = receiveHelper(*sock);

        // eliminate unused variable warning when debug statements are commented
        (void)routingId;

        // Debug statements
        // std::cout << "Version[" << version << "]\n";
        // std::cout << "Sequence[" << sequence << "]\n";
        // std::cout << "Domain[" << domain << "]\n";
        // std::cout << "Address[" << address << "]\n";
        // std::cout << "Routing Id[" << routingId << "]\n";
        // std::cout << "Mechanism[" << mechanism << "]\n";
        // std::cout << "Username[" << givenUsername << "] [" << user << "]\n";
        // std::cout << "Pass[" << givenPassword << "] [" << pass << "]\n";

        // Check that we received some kind of address. This could be used
        // in the future to only accept connections from specific addresses.
        if (address.empty())
        {
          sendAuthErrorHelper(*sock, "Invalid address");
          continue;
        }

        // Check the version
        if (version != "1.0")
        {
          sendAuthErrorHelper(*sock, "Invalid version");
          continue;
        }

        // Check the mechanism
        if (mechanism != "PLAIN")
        {
          sendAuthErrorHelper(*sock, "Invalid mechanism");
          continue;
        }

        // Check the domain
        if (std::strcmp(domain.c_str(), kIgnAuthDomain) != 0)
        {
          sendAuthErrorHelper(*sock, "Invalid domain");
          continue;
        }

#ifdef IGN_ZMQ_POST_4_3_1
        sendHelper(*sock, version, zmq::send_flags::sndmore);
        sendHelper(*sock, sequence, zmq::send_flags::sndmore);
#else
        sendHelper(*sock, version, ZMQ_SNDMORE);
        sendHelper(*sock, sequence, ZMQ_SNDMORE);
#endif

        // Check the username and password
        if (givenUsername == user && givenPassword == pass)
        {
#ifdef IGN_ZMQ_POST_4_3_1
          sendHelper(*sock, "200", zmq::send_flags::sndmore);
          sendHelper(*sock, "OK", zmq::send_flags::sndmore);
          sendHelper(*sock, "anonymous", zmq::send_flags::sndmore);
          sendHelper(*sock, "", zmq::send_flags::none);
#else
          sendHelper(*sock, "200", ZMQ_SNDMORE);
          sendHelper(*sock, "OK", ZMQ_SNDMORE);
          sendHelper(*sock, "anonymous", ZMQ_SNDMORE);
          sendHelper(*sock, "", 0);
#endif
        }
        else
        {
          sendAuthErrorHelper(*sock, "Invalid username or password");
        }
      }
    }
  }
  catch (...)
  {
    // This catch can be triggered when ctrl-c is pressed and the context is
    // deleted. Capture this case, and quit gracefully.
  }

  sock->close();
}

/////////////////////////////////////////////////
void NodeSharedPrivate::PublishThread()
{
  // Loop until exits
  while (!this->exit)
  {
    std::unique_ptr<PublishMsgDetails> msgDetails = nullptr;
    // Lock the mutex, and acquire the next message to be published.
    {
      std::unique_lock<std::mutex> queueLock(this->pubThreadMutex);

      // Wait for more messages if the queue is empty. Otherwise get the
      // next message and continue.
      if (this->pubQueue.empty())
      {
        auto now = std::chrono::system_clock::now();
        this->signalNewPub.wait_until(queueLock, now + 500ms,
          [&]{return !this->pubQueue.empty() || this->exit;});
      }

      if (this->pubQueue.empty())
        continue;

      // Stop early on exit.
      if (this->exit)
        break;

      // Get the message
      msgDetails = std::move(this->pubQueue.front());
      this->pubQueue.pop();
    }

    // Send the message to all the local handlers.
    for (auto &handler : msgDetails->localHandlers)
    {
      try
      {
        handler->RunLocalCallback(*(msgDetails->msgCopy.get()),
            msgDetails->info);
      }
      catch (...)
      {
        std::cerr << "Exception occurred in a local callback "
          << "on topic [" << msgDetails->info.Topic() << "] with message ["
          << msgDetails->msgCopy->DebugString() << "]" << std::endl;
      }
    }

    // Send the message to all the raw handlers.
    for (auto &handler : msgDetails->rawHandlers)
    {
      try
      {
        handler->RunRawCallback(msgDetails->sharedBuffer.get(),
            msgDetails->msgSize, msgDetails->info);
      }
      catch (...)
      {
        std::cerr << "Exception occured in a local raw callback "
          << "on topic [" << msgDetails->info.Topic() << "] with "
          << "message [" << msgDetails->msgCopy->DebugString() << "]"
          << std::endl;
      }
    }
  }
}
