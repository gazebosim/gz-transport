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
# pragma warning(push, 0)
#endif
#ifdef _WIN32
  // For socket(), connect(), send(), and recv().
  #include <Winsock2.h>
  #include <Ws2def.h>
  #include <Ws2ipdef.h>
  #include <Ws2tcpip.h>
  // Type used for raw data on this platform.
  typedef char raw_type;
#else
  // For data types
  #include <sys/types.h>
  // For socket(), connect(), send(), and recv()
  #include <sys/socket.h>
  // For gethostbyname()
  #include <netdb.h>
  // For inet_addr()
  #include <arpa/inet.h>
  // For close()
  #include <unistd.h>
  // For sockaddr_in
  #include <netinet/in.h>
  // Type used for raw data on this platform
  typedef void raw_type;
#endif

#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/NetUtils.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

#ifdef _WIN32
  static bool initialized = false;
#endif

//////////////////////////////////////////////////
Discovery::Discovery(const std::string &_pUuid, bool _verbose)
: dataPtr(new DiscoveryPrivate())
{
  // Initialization
  this->dataPtr->pUuid = _pUuid;
  this->dataPtr->silenceInterval = this->dataPtr->DefSilenceInterval;
  this->dataPtr->activityInterval = this->dataPtr->DefActivityInterval;
  this->dataPtr->advertiseInterval = this->dataPtr->DefAdvertiseInterval;
  this->dataPtr->heartbeatInterval = this->dataPtr->DefHeartbeatInterval;
  this->dataPtr->connectionCb = nullptr;
  this->dataPtr->disconnectionCb = nullptr;
  this->dataPtr->verbose = _verbose;
  this->dataPtr->exit = false;

  // Get this host IP address.
  this->dataPtr->hostAddr = determineHost();

#ifdef _WIN32
  if (!initialized)
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    // Request WinSock v2.0.
    wVersionRequested = MAKEWORD(2, 0);
    // Load WinSock DLL.
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
     std::cerr << "Unable to load WinSock DLL" << std::endl;
     return;
    }

    initialized = true;
  }
#endif

  // Make a new socket for sending/receiving discovery information.
  if ((this->dataPtr->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    std::cerr << "Socket creation failed." << std::endl;
    return;
  }

  // Socket option: SO_REUSEADDR.
  int reuseAddr = 1;
  if (setsockopt(this->dataPtr->sock, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<const char *>(&reuseAddr), sizeof(reuseAddr)) != 0)
  {
    std::cerr << "Error setting socket option (SO_REUSEADDR)." << std::endl;
    return;
  }

#ifdef SO_REUSEPORT
  // Socket option: SO_REUSEPORT.
  int reusePort = 1;
  if (setsockopt(this->dataPtr->sock, SOL_SOCKET, SO_REUSEPORT,
        reinterpret_cast<const char *>(&reusePort), sizeof(reusePort)) != 0)
  {
    std::cerr << "Error setting socket option (SO_REUSEPORT)." << std::endl;
    return;
  }
#endif

  // Socket option: IP_MULTICAST_IF.
  // This option selects the source interface for outgoing messages.
  struct in_addr ifAddr;
  ifAddr.s_addr = inet_addr(this->dataPtr->hostAddr.c_str());
  if (setsockopt(this->dataPtr->sock, IPPROTO_IP, IP_MULTICAST_IF,
    reinterpret_cast<const char*>(&ifAddr), sizeof(ifAddr)) != 0)
  {
    std::cerr << "Error setting socket option (IP_MULTICAST_IF)." << std::endl;
    return;
  }

  // Join the multicast group
  struct ip_mreq group;
  group.imr_multiaddr.s_addr = inet_addr(this->dataPtr->MulticastGroup.c_str());
  group.imr_interface.s_addr = inet_addr(this->dataPtr->hostAddr.c_str());
  if (setsockopt(this->dataPtr->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
    reinterpret_cast<const char*>(&group), sizeof(group)) != 0)
  {
    std::cerr << "Error setting socket option (IP_ADD_MEMBERSHIP)."
              << std::endl;
    return;
  }

  // Bind the socket to the discovery port.
  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(this->dataPtr->DiscoveryPort);

  if (bind(this->dataPtr->sock, reinterpret_cast<sockaddr *>(&localAddr),
        sizeof(sockaddr_in)) < 0)
  {
    std::cerr << "Binding to a local port failed." << std::endl;
    return;
  }

  // Set 'mcastAddr' to the multicast discovery group.
  memset(&this->dataPtr->mcastAddr, 0, sizeof(this->dataPtr->mcastAddr));
  this->dataPtr->mcastAddr.sin_family = AF_INET;
  this->dataPtr->mcastAddr.sin_addr.s_addr =
    inet_addr(this->dataPtr->MulticastGroup.c_str());
  this->dataPtr->mcastAddr.sin_port = htons(this->dataPtr->DiscoveryPort);

  // Start the thread that receives discovery information.
  this->dataPtr->threadReception =
    new std::thread(&Discovery::RunReceptionTask, this);

  // Start the thread that sends heartbeats.
  this->dataPtr->threadHeartbeat =
    new std::thread(&Discovery::RunHeartbeatTask, this);

  // Start the thread that checks the topic information validity.
  this->dataPtr->threadActivity =
    new std::thread(&Discovery::RunActivityTask, this);

  if (this->dataPtr->verbose)
    this->PrintCurrentState();
}

//////////////////////////////////////////////////
Discovery::~Discovery()
{
  // Tell the service thread to terminate.
  this->dataPtr->exitMutex.lock();
  this->dataPtr->exit = true;
  this->dataPtr->exitMutex.unlock();

  // Don't join on Windows, because it can hang when this object
  // is destructed on process exit (e.g., when it's a global static).
  // I think that it's due to this bug:
  // https://connect.microsoft.com/VisualStudio/feedback/details/747145/std-thread-join-hangs-if-called-after-main-exits-when-using-vs2012-rc
#ifndef _WIN32
  // Wait for the service threads to finish before exit.
  this->dataPtr->threadReception->join();
  this->dataPtr->threadHeartbeat->join();
  this->dataPtr->threadActivity->join();
#endif

  // Broadcast a BYE message to trigger the remote cancellation of
  // all our advertised topics.
  this->SendMsg(ByeType, "", "", "", "", Scope::All);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Close sockets.
#ifdef _WIN32
  closesocket(this->dataPtr->sock);
#else
  close(this->dataPtr->sock);
#endif
}

//////////////////////////////////////////////////
void Discovery::Advertise(const MsgType &_advType, const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_nUuid,
  const Scope &_scope)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // Add the addressing information (local node).
  if (_advType == MsgType::Msg)
  {
    this->dataPtr->infoMsg.AddAddress(
      _topic, _addr, _ctrl, this->dataPtr->pUuid, _nUuid, _scope);
  }
  else
  {
    this->dataPtr->infoSrv.AddAddress(
      _topic, _addr, _ctrl, this->dataPtr->pUuid, _nUuid, _scope);
  }

  // If the scope is 'Process', do not advertise a message outside this process.
  if (_scope == Scope::Process)
    return;

  // Broadcast periodically my topic information.
  if (_advType == MsgType::Msg)
    this->SendMsg(AdvType, _topic, _addr, _ctrl, _nUuid, _scope);
  else
    this->SendMsg(AdvSrvType, _topic, _addr, _ctrl, _nUuid, _scope);
}

//////////////////////////////////////////////////
void Discovery::Unadvertise(const MsgType &_unadvType,
  const std::string &_topic, const std::string &_nUuid)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  uint8_t msgType;
  TopicStorage *storage;

  if (_unadvType == MsgType::Msg)
  {
    msgType = UnadvType;
    storage = &this->dataPtr->infoMsg;
  }
  else
  {
    msgType = UnadvSrvType;
    storage = &this->dataPtr->infoSrv;
  }

  Address_t inf;
  // Don't do anything if the topic is not advertised by any of my nodes.
  if (!storage->GetAddress(_topic, this->dataPtr->pUuid, _nUuid, inf))
    return;

  // Remove the topic information.
  storage->DelAddressByNode(_topic, this->dataPtr->pUuid, _nUuid);

  // Do not advertise a message outside the process if the scope is 'Process'.
  if (inf.scope == Scope::Process)
    return;

  this->SendMsg(msgType, _topic, inf.addr, inf.ctrl, _nUuid, inf.scope);
}

//////////////////////////////////////////////////
void Discovery::Discover(const std::string &_topic, bool _isSrv)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  uint8_t msgType;
  TopicStorage *storage;
  DiscoveryCallback cb;

  if (_isSrv)
  {
    msgType = SubSrvType;
    storage = &this->dataPtr->infoSrv;
    cb = this->dataPtr->connectionSrvCb;
  }
  else
  {
    msgType = SubType;
    storage = &this->dataPtr->infoMsg;
    cb = this->dataPtr->connectionCb;
  }

  // Broadcast a discovery request for this service call.
  this->SendMsg(msgType, _topic, "", "", "", Scope::All);

  // I already have information about this topic.
  if (storage->HasTopic(_topic))
  {
    Addresses_M addresses;
    if (storage->GetAddresses(_topic, addresses))
    {
      for (auto &proc : addresses)
      {
        for (auto &node : proc.second)
        {
          if (cb)
          {
            // Execute the user's callback for a service request. Notice
            // that we only execute one callback for preventing receive multiple
            // service responses for a single request.
            cb(_topic, node.addr, node.ctrl, proc.first, node.nUuid,
               node.scope);
          }
        }
      }
    }
  }
}

//////////////////////////////////////////////////
bool Discovery::GetMsgAddresses(const std::string &_topic,
                                Addresses_M &_addresses)
{
  return this->dataPtr->infoMsg.GetAddresses(_topic, _addresses);
}

//////////////////////////////////////////////////
bool Discovery::GetSrvAddresses(const std::string &_topic,
                                Addresses_M &_addresses)
{
  return this->dataPtr->infoSrv.GetAddresses(_topic, _addresses);
}

//////////////////////////////////////////////////
std::string Discovery::GetHostAddr() const
{
  return this->dataPtr->hostAddr;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetActivityInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->activityInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetHeartbeatInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->heartbeatInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetAdvertiseInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->advertiseInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::GetSilenceInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->silenceInterval;
}

//////////////////////////////////////////////////
void Discovery::SetActivityInterval(const unsigned int _ms)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->activityInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetHeartbeatInterval(const unsigned int _ms)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->heartbeatInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetAdvertiseInterval(const unsigned int _ms)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->advertiseInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetSilenceInterval(const unsigned int _ms)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->silenceInterval = _ms;
}

//////////////////////////////////////////////////
void Discovery::SetConnectionsCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetDisconnectionsCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetConnectionsSrvCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionSrvCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::SetDisconnectionsSrvCb(const DiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionSrvCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::RunActivityTask()
{
  while (true)
  {
    this->dataPtr->mutex.lock();

    Timestamp now = std::chrono::steady_clock::now();
    for (auto it = this->dataPtr->activity.cbegin();
           it != this->dataPtr->activity.cend();)
    {
      // Elapsed time since the last update from this publisher.
      std::chrono::duration<double> elapsed = now - it->second;

      // This publisher has expired.
      if (std::chrono::duration_cast<std::chrono::milliseconds>
           (elapsed).count() > this->dataPtr->silenceInterval)
      {
        // Remove all the info entries for this process UUID.
        this->dataPtr->infoMsg.DelAddressesByProc(it->first);
        this->dataPtr->infoSrv.DelAddressesByProc(it->first);

        // Notify without topic information. This is useful to inform the client
        // that a remote node is gone, even if we were not interested in its
        // topics.
        this->dataPtr->disconnectionCb("", "", "", it->first, "", Scope::All);

        // Remove the activity entry.
        this->dataPtr->activity.erase(it++);
      }
      else
        ++it;
    }
    this->dataPtr->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->dataPtr->activityInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
      if (this->dataPtr->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void Discovery::RunHeartbeatTask()
{
  while (true)
  {
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

      this->SendMsg(HeartbeatType, "", "", "", "", Scope::All);

      // Re-advertise topics that are advertised inside this process.
      std::map<std::string, std::vector<Address_t>> nodes;
      this->dataPtr->infoMsg.GetAddressesByProc(this->dataPtr->pUuid, nodes);
      for (auto &topic : nodes)
      {
        for (auto &node : topic.second)
        {
          this->SendMsg(AdvType, topic.first, node.addr, node.ctrl, node.nUuid,
            node.scope);
        }
      }

      // Re-advertise services that are advertised inside this process.
      this->dataPtr->infoSrv.GetAddressesByProc(this->dataPtr->pUuid, nodes);
      for (auto &topic : nodes)
      {
        for (auto &node : topic.second)
        {
          this->SendMsg(AdvSrvType, topic.first, node.addr, node.ctrl,
            node.nUuid, node.scope);
        }
      }
    }

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->dataPtr->heartbeatInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
      if (this->dataPtr->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void Discovery::RunReceptionTask()
{
  while (true)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {0, this->dataPtr->sock, ZMQ_POLLIN, 0},
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]),
      this->dataPtr->Timeout);

    //  If we got a reply, process it.
    if (items[0].revents & ZMQ_POLLIN)
    {
      this->RecvDiscoveryUpdate();

      if (this->dataPtr->verbose)
        this->PrintCurrentState();
    }

    // Is it time to exit?
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
      if (this->dataPtr->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void Discovery::RecvDiscoveryUpdate()
{
  char rcvStr[DiscoveryPrivate::MaxRcvStr];
  std::string srcAddr;
  uint16_t srcPort;
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);

  if ((recvfrom(this->dataPtr->sock, reinterpret_cast<raw_type *>(rcvStr),
        DiscoveryPrivate::MaxRcvStr, 0, reinterpret_cast<sockaddr *>(&clntAddr),
        reinterpret_cast<socklen_t *>(&addrLen))) < 0)
  {
    std:: cerr << "Receive failed" << std::endl;
    return;
  }
  srcAddr = inet_ntoa(clntAddr.sin_addr);
  srcPort = ntohs(clntAddr.sin_port);

  if (this->dataPtr->verbose)
  {
    std::cout << "\nReceived discovery update from " << srcAddr << ": "
              << srcPort << std::endl;
  }

  this->DispatchDiscoveryMsg(srcAddr, rcvStr);
}

//////////////////////////////////////////////////
void Discovery::DispatchDiscoveryMsg(const std::string &_fromIp, char *_msg)
{
  Header header;
  char *pBody = _msg;

  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // Create the header from the raw bytes.
  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  auto recvPUuid = header.GetPUuid();

  // Discard our own discovery messages.
  if (recvPUuid == this->dataPtr->pUuid)
    return;

  // Update timestamp.
  this->dataPtr->activity[recvPUuid] = std::chrono::steady_clock::now();

  switch (header.GetType())
  {
    case AdvType:
    case AdvSrvType:
    {
      // Read the rest of the fields.
      AdvertiseMsg advMsg;
      advMsg.UnpackBody(pBody);
      auto recvTopic = advMsg.GetTopic();
      auto recvAddr = advMsg.GetAddress();
      auto recvCtrl = advMsg.GetControlAddress();
      auto recvNUuid = advMsg.GetNodeUuid();
      auto recvScope = advMsg.GetScope();

      // Check scope of the topic.
      if ((recvScope == Scope::Process) ||
          (recvScope == Scope::Host && _fromIp != this->dataPtr->hostAddr))
      {
        return;
      }

      DiscoveryCallback cb;
      TopicStorage *storage;

      if (header.GetType() == AdvType)
      {
        storage = &this->dataPtr->infoMsg;
        cb = this->dataPtr->connectionCb;
      }
      else
      {
        storage = &this->dataPtr->infoSrv;
        cb = this->dataPtr->connectionSrvCb;
      }

      // Register an advertised address for the topic.
      bool added = storage->AddAddress(recvTopic, recvAddr, recvCtrl, recvPUuid,
        recvNUuid, recvScope);

      if (added && cb)
      {
        // Execute the client's callback.
        cb(recvTopic, recvAddr, recvCtrl, recvPUuid, recvNUuid, recvScope);
      }

      break;
    }
    case SubType:
    case SubSrvType:
    {
      // Read the rest of the fields.
      SubscriptionMsg subMsg;
      subMsg.UnpackBody(pBody);
      auto recvTopic = subMsg.GetTopic();

      uint8_t msgType;
      TopicStorage *storage;

      if (header.GetType() == SubType)
      {
        msgType = AdvType;
        storage = &this->dataPtr->infoMsg;
      }
      else
      {
        msgType = AdvSrvType;
        storage = &this->dataPtr->infoSrv;
      }

      // Check if at least one of my nodes advertises the topic requested.
      if (storage->HasAnyAddresses(recvTopic, this->dataPtr->pUuid))
      {
        Addresses_M addresses;
        if (storage->GetAddresses(recvTopic, addresses))
        {
          for (auto nodeInfo : addresses[this->dataPtr->pUuid])
          {
            // Check scope of the topic.
            if ((nodeInfo.scope == Scope::Process) ||
                (nodeInfo.scope == Scope::Host &&
                 _fromIp != this->dataPtr->hostAddr))
            {
              continue;
            }

            // Answer an ADVERTISE message.
            this->SendMsg(msgType, recvTopic, nodeInfo.addr, nodeInfo.ctrl,
              nodeInfo.nUuid, nodeInfo.scope);
          }
        }
      }

      break;
    }
    case HeartbeatType:
    {
      // The timestamp has already been updated.
      break;
    }
    case ByeType:
    {
      // Remove the activity entry for this publisher.
      this->dataPtr->activity.erase(recvPUuid);

      if (this->dataPtr->disconnectionCb)
      {
        // Notify the new disconnection.
        this->dataPtr->disconnectionCb("", "", "", recvPUuid, "", Scope::All);
      }

      if (this->dataPtr->disconnectionSrvCb)
      {
        // Notify the new disconnection.
        this->dataPtr->disconnectionSrvCb("", "", "", recvPUuid, "",
          Scope::All);
      }

      // Remove the address entry for this topic.
      this->dataPtr->infoMsg.DelAddressesByProc(recvPUuid);
      this->dataPtr->infoSrv.DelAddressesByProc(recvPUuid);

      break;
    }
    case UnadvType:
    case UnadvSrvType:
    {
      // Read the address.
      AdvertiseMsg advMsg;
      advMsg.UnpackBody(pBody);
      auto recvTopic = advMsg.GetTopic();
      auto recvAddr = advMsg.GetAddress();
      auto recvCtrl = advMsg.GetControlAddress();
      auto recvNUuid = advMsg.GetNodeUuid();
      auto recvScope = advMsg.GetScope();

      // Check scope of the topic.
      if ((recvScope == Scope::Process) ||
          (recvScope == Scope::Host && _fromIp != this->dataPtr->hostAddr))
      {
        return;
      }

      DiscoveryCallback cb;
      TopicStorage *storage;

      if (header.GetType() == UnadvType)
      {
        storage = &this->dataPtr->infoMsg;
        cb = this->dataPtr->disconnectionCb;
      }
      else
      {
        storage = &this->dataPtr->infoSrv;
        cb = this->dataPtr->disconnectionSrvCb;
      }

      if (cb)
      {
        // Notify the new disconnection.
        cb(recvTopic, recvAddr, recvCtrl, recvPUuid, recvNUuid, recvScope);
      }

      // Remove the address entry for this topic.
      storage->DelAddressByNode(recvTopic, recvPUuid, recvNUuid);

      break;
    }
    default:
    {
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
    }
  }
}

//////////////////////////////////////////////////
void Discovery::SendMsg(uint8_t _type, const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_nUuid,
  const Scope &_scope, int _flags)
{
  // Create the header.
  Header header(DiscoveryPrivate::Version, this->dataPtr->pUuid, _type, _flags);
  auto msgLength = 0;
  std::vector<char> buffer;

  switch (_type)
  {
    case AdvType:
    case UnadvType:
    case AdvSrvType:
    case UnadvSrvType:
    {
      // Create the [UN]ADVERTISE message.
      AdvertiseMsg advMsg(header, _topic, _addr, _ctrl, _nUuid, _scope,
        "not used");

      // Allocate a buffer and serialize the message.
      buffer.resize(advMsg.GetMsgLength());
      advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
      msgLength = advMsg.GetMsgLength();
      break;
    }
    case SubType:
    case SubSrvType:
    {
      // Create the [UN]SUBSCRIBE message.
      SubscriptionMsg subMsg(header, _topic);

      // Allocate a buffer and serialize the message.
      buffer.resize(subMsg.GetMsgLength());
      subMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
      msgLength = subMsg.GetMsgLength();
      break;
    }
    case HeartbeatType:
    case ByeType:
    {
      // Allocate a buffer and serialize the message.
      buffer.resize(header.GetHeaderLength());
      header.Pack(reinterpret_cast<char*>(&buffer[0]));
      msgLength = header.GetHeaderLength();
      break;
    }
    default:
      std::cerr << "Discovery::SendMsg() error: Unrecognized message"
                << " type [" << _type << "]" << std::endl;
      return;
  }

  // Send the discovery message to the multicast group.
  if (sendto(this->dataPtr->sock, reinterpret_cast<const raw_type *>(
    reinterpret_cast<unsigned char*>(&buffer[0])),
    msgLength, 0, reinterpret_cast<sockaddr *>(&this->dataPtr->mcastAddr),
    sizeof(this->dataPtr->mcastAddr)) != msgLength)
  {
    std::cerr << "Exception sending a message" << std::endl;
    return;
  }

  if (this->dataPtr->verbose)
  {
    std::cout << "\t* Sending " << MsgTypesStr[_type]
              << " msg [" << _topic << "]" << std::endl;
  }
}

//////////////////////////////////////////////////
void Discovery::PrintCurrentState()
{
  std::cout << "---------------" << std::endl;
  std::cout << "Discovery state" << std::endl;
  std::cout << "\tUUID: " << this->dataPtr->pUuid << std::endl;
  std::cout << "Settings" << std::endl;
  std::cout << "\tActivity: " << this->dataPtr->activityInterval
            << " ms." << std::endl;
  std::cout << "\tHeartbeat: " << this->dataPtr->heartbeatInterval
            << "ms." << std::endl;
  std::cout << "\tRetrans.: " << this->dataPtr->advertiseInterval
            << " ms." << std::endl;
  std::cout << "\tSilence: " << this->dataPtr->silenceInterval
            << " ms." << std::endl;
  std::cout << "Known msgs" << std::endl;
  this->dataPtr->infoMsg.Print();
  std::cout << "Known services" << std::endl;
  this->dataPtr->infoSrv.Print();

  // Used to calculate the elapsed time.
  Timestamp now = std::chrono::steady_clock::now();

  std::cout << "Activity" << std::endl;
  if (this->dataPtr->activity.empty())
    std::cout << "\t<empty>" << std::endl;
  else
  {
    for (auto &proc : this->dataPtr->activity)
    {
      // Elapsed time since the last update from this publisher.
      std::chrono::duration<double> elapsed = now - proc.second;

      std::cout << "\t" << proc.first << std::endl;
      std::cout << "\t\t" << "Since: " << std::chrono::duration_cast<
        std::chrono::milliseconds>(elapsed).count() << " ms. ago. "
        << std::endl;
    }
  }
  std::cout << "---------------" << std::endl;
}

//////////////////////////////////////////////////
void Discovery::GetTopicList(std::vector<std::string> &_topics) const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->infoMsg.GetTopicList(_topics);
}

//////////////////////////////////////////////////
void Discovery::GetServiceList(std::vector<std::string> &_services) const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->infoSrv.GetTopicList(_services);
}

//////////////////////////////////////////////////
std::recursive_mutex& Discovery::GetMutex()
{
  return this->dataPtr->mutex;
}
