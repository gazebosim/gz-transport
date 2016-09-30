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

#ifdef _WIN32
  // For socket(), connect(), send(), and recv().
  #include <Winsock2.h>
  #include <Ws2def.h>
  #include <Ws2ipdef.h>
  #include <Ws2tcpip.h>
  // Type used for raw data on this platform.
  using raw_type = char;
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
  using raw_type = void;
#endif

#ifdef _MSC_VER
  #pragma warning(push, 0)
#endif
#include <zmq.hpp>
#ifdef _WIN32
  #pragma warning(pop)
  // Suppress "decorated name length exceed" warning in STL.
  #pragma warning(disable: 4503)
  // Suppress "depreted API warnings" in WINSOCK.
  #pragma warning(disable: 4996)
#endif

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "ignition/transport/AdvertiseOptions.hh"
#include "ignition/transport/Discovery.hh"
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NetUtils.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TopicStorage.hh"
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
  this->dataPtr->enabled = false;

  // Get this host IP address.
  this->dataPtr->hostAddr = determineHost();

  std::string ignIp;
  if (env("IGN_IP", ignIp) && !ignIp.empty())
    this->dataPtr->hostInterfaces = {ignIp};
  else
  {
    // Get the list of network interfaces in this host.
    this->dataPtr->hostInterfaces = determineInterfaces();
  }

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

  for (const auto &netIface : this->dataPtr->hostInterfaces)
  {
    auto succeed = this->RegisterNetIface(netIface);

    // If the IP address that we're selecting as the main IP address of the host
    // is invalid, we change it to 127.0.0.1 . This is probably because IGN_IP
    // is set to a wrong value.
    if (netIface == this->dataPtr->hostAddr && !succeed)
    {
      this->RegisterNetIface("127.0.0.1");
      std::cerr << "Did you set the environment variable IGN_IP with a correct "
                << "IP address? " << std::endl
                << "  [" << netIface << "] seems an invalid local IP address."
                << std::endl
                << "  Using 127.0.0.1 as hostname." << std::endl;
      this->dataPtr->hostAddr = "127.0.0.1";
    }
  }

  // Socket option: SO_REUSEADDR. This options is used only for receiving data.
  // We can reuse the same socket for receiving multicast data from multiple
  // interfaces. We will use the socket at position 0 for receiving data.
  int reuseAddr = 1;
  if (setsockopt(this->dataPtr->sockets.at(0), SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<const char *>(&reuseAddr), sizeof(reuseAddr)) != 0)
  {
    std::cerr << "Error setting socket option (SO_REUSEADDR)." << std::endl;
    return;
  }

#ifdef SO_REUSEPORT
  // Socket option: SO_REUSEPORT. This options is used only for receiving data.
  // We can reuse the same socket for receiving multicast data from multiple
  // interfaces. We will use the socket at position 0 for receiving data.
  int reusePort = 1;
  if (setsockopt(this->dataPtr->sockets.at(0), SOL_SOCKET, SO_REUSEPORT,
        reinterpret_cast<const char *>(&reusePort), sizeof(reusePort)) != 0)
  {
    std::cerr << "Error setting socket option (SO_REUSEPORT)." << std::endl;
    return;
  }
#endif

  // Bind the first socket to the discovery port.
  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port =
      htons(static_cast<u_short>(this->dataPtr->DiscoveryPort));

  if (bind(this->dataPtr->sockets.at(0),
    reinterpret_cast<sockaddr *>(&localAddr), sizeof(sockaddr_in)) < 0)
  {
    std::cerr << "Binding to a local port failed." << std::endl;
    return;
  }

  // Set 'mcastAddr' to the multicast discovery group.
  memset(&this->dataPtr->mcastAddr, 0, sizeof(this->dataPtr->mcastAddr));
  this->dataPtr->mcastAddr.sin_family = AF_INET;
  this->dataPtr->mcastAddr.sin_addr.s_addr =
    inet_addr(this->dataPtr->MulticastGroup.c_str());
  this->dataPtr->mcastAddr.sin_port =
      htons(static_cast<u_short>(this->dataPtr->DiscoveryPort));

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
  if (this->dataPtr->threadReception.joinable())
    this->dataPtr->threadReception.join();

  if (this->dataPtr->threadHeartbeat.joinable())
    this->dataPtr->threadHeartbeat.join();

  if (this->dataPtr->threadActivity.joinable())
    this->dataPtr->threadActivity.join();
#else
  bool exitLoop = false;
  while (!exitLoop)
  {
    std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
    {
      if (this->dataPtr->threadReceptionExiting &&
          this->dataPtr->threadHeartbeatExiting &&
          this->dataPtr->threadActivityExiting)
      {
        exitLoop = true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
#endif

  // Broadcast a BYE message to trigger the remote cancellation of
  // all our advertised topics.
  this->SendMsg(ByeType,
    Publisher("", "", this->dataPtr->pUuid, "", Scope_t::ALL));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Close sockets.
  for (const auto &sock : this->dataPtr->sockets)
  {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
  }
}

//////////////////////////////////////////////////
void Discovery::Start()
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  // The service is already running.
  if (this->dataPtr->enabled)
    return;

  this->dataPtr->enabled = true;

  // Start the thread that receives discovery information.
  this->dataPtr->threadReception =
    std::thread(&Discovery::RunReceptionTask, this);

  // Start the thread that sends heartbeats.
  this->dataPtr->threadHeartbeat =
    std::thread(&Discovery::RunHeartbeatTask, this);

  // Start the thread that checks the topic information validity.
  this->dataPtr->threadActivity =
    std::thread(&Discovery::RunActivityTask, this);

#ifdef _WIN32
  this->dataPtr->threadReceptionExiting = false;
  this->dataPtr->threadHeartbeatExiting = false;
  this->dataPtr->threadActivityExiting = false;
  this->dataPtr->threadReception.detach();
  this->dataPtr->threadHeartbeat.detach();
  this->dataPtr->threadActivity.detach();
#endif
}

//////////////////////////////////////////////////
bool Discovery::AdvertiseMsg(const MessagePublisher &_publisher)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  if (!this->dataPtr->enabled)
    return false;

  // Add the addressing information (local publisher).
  if (!this->dataPtr->infoMsg.AddPublisher(_publisher))
    return false;

  // Only advertise a message outside this process if the scope is not 'Process'
  if (_publisher.Scope() != Scope_t::PROCESS)
    this->SendMsg(AdvType, _publisher);

  return true;
}

//////////////////////////////////////////////////
bool Discovery::AdvertiseSrv(const ServicePublisher &_publisher)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  if (!this->dataPtr->enabled)
    return false;

  // Add the addressing information (local publisher).
  if (!this->dataPtr->infoSrv.AddPublisher(_publisher))
    return false;

  // Only advertise a message outside this process if the scope is not 'Process'
  if (_publisher.Scope() != Scope_t::PROCESS)
    this->SendMsg(AdvSrvType, _publisher);

  return true;
}

//////////////////////////////////////////////////
bool Discovery::UnadvertiseMsg(const std::string &_topic,
  const std::string &_nUuid)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  if (!this->dataPtr->enabled)
    return false;

  MessagePublisher inf;
  // Don't do anything if the topic is not advertised by any of my nodes.
  if (!this->dataPtr->infoMsg.Publisher(_topic, this->dataPtr->pUuid, _nUuid,
    inf))
  {
    return true;
  }

  // Remove the topic information.
  this->dataPtr->infoMsg.DelPublisherByNode(_topic, this->dataPtr->pUuid,
    _nUuid);

  // Only unadvertise a message outside this process if the scope
  // is not 'Process'.
  if (inf.Scope() != Scope_t::PROCESS)
    this->SendMsg(UnadvType, inf);

  return true;
}

//////////////////////////////////////////////////
bool Discovery::UnadvertiseSrv(const std::string &_topic,
  const std::string &_nUuid)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  if (!this->dataPtr->enabled)
    return false;

  ServicePublisher inf;
  // Don't do anything if the topic is not advertised by any of my nodes.
  if (!this->dataPtr->infoSrv.Publisher(_topic, this->dataPtr->pUuid, _nUuid,
    inf))
  {
    return true;
  }

  // Remove the topic information.
  this->dataPtr->infoSrv.DelPublisherByNode(_topic, this->dataPtr->pUuid,
    _nUuid);

  // Only unadvertise a message outside this process if the scope
  // is not 'Process'.
  if (inf.Scope() != Scope_t::PROCESS)
    this->SendMsg(UnadvSrvType, inf);

  return true;
}

//////////////////////////////////////////////////
bool Discovery::DiscoverMsg(const std::string &_topic)
{
  MsgDiscoveryCallback cb;
  bool found;
  MsgAddresses_M addresses;

  {
    std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

    if (!this->dataPtr->enabled)
      return false;

    cb = this->dataPtr->connectionCb;

    MessagePublisher pub;
    pub.SetTopic(_topic);
    pub.SetPUuid(this->dataPtr->pUuid);
    pub.SetScope(Scope_t::ALL);

    // Broadcast a discovery request for this service call.
    this->SendMsg(SubType, pub);

    found = this->dataPtr->infoMsg.Publishers(_topic, addresses);
  }

  if (found)
  {
    // I already have information about this topic.
    for (auto &proc : addresses)
    {
      for (auto &node : proc.second)
      {
        if (cb)
        {
          // Execute the user's callback for a service request. Notice
          // that we only execute one callback for preventing receive multiple
          // service responses for a single request.
          cb(node);
        }
      }
    }
  }

  return true;
}

//////////////////////////////////////////////////
bool Discovery::DiscoverSrv(const std::string &_topic)
{
  SrvDiscoveryCallback cb;
  SrvAddresses_M addresses;
  bool found;

  {
    std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

    if (!this->dataPtr->enabled)
      return false;

    cb = this->dataPtr->connectionSrvCb;

    ServicePublisher pub;
    pub.SetTopic(_topic);
    pub.SetPUuid(this->dataPtr->pUuid);
    pub.SetScope(Scope_t::ALL);

    // Broadcast a discovery request for this service call.
    this->SendMsg(SubSrvType, pub);

    found = this->dataPtr->infoSrv.Publishers(_topic, addresses);
  }

  if (found)
  {
    // I already have information about this service.
    for (auto &proc : addresses)
    {
      for (auto &node : proc.second)
      {
        if (cb)
        {
          // Execute the user's callback for a service request. Notice
          // that we only execute one callback for preventing receive multiple
          // service responses for a single request.
          cb(node);
        }
      }
    }
  }

  return true;
}

//////////////////////////////////////////////////
const TopicStorage<MessagePublisher> &Discovery::DiscoveryMsgInfo() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->infoMsg;
}

//////////////////////////////////////////////////
const TopicStorage<ServicePublisher> &Discovery::DiscoverySrvInfo() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->infoSrv;
}

//////////////////////////////////////////////////
bool Discovery::MsgPublishers(const std::string &_topic,
                              MsgAddresses_M &_publishers)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->infoMsg.Publishers(_topic, _publishers);
}

//////////////////////////////////////////////////
bool Discovery::SrvPublishers(const std::string &_topic,
                              SrvAddresses_M &_publishers)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->infoSrv.Publishers(_topic, _publishers);
}

//////////////////////////////////////////////////
std::string Discovery::HostAddr() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->hostAddr;
}

//////////////////////////////////////////////////
unsigned int Discovery::ActivityInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->activityInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::HeartbeatInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->heartbeatInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::AdvertiseInterval() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  return this->dataPtr->advertiseInterval;
}

//////////////////////////////////////////////////
unsigned int Discovery::SilenceInterval() const
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
void Discovery::ConnectionsCb(const MsgDiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::DisconnectionsCb(const MsgDiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::ConnectionsSrvCb(const SrvDiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->connectionSrvCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::DisconnectionsSrvCb(const SrvDiscoveryCallback &_cb)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->disconnectionSrvCb = _cb;
}

//////////////////////////////////////////////////
void Discovery::RunActivityTask()
{
  bool timeToExit = false;
  while (!timeToExit)
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
        this->dataPtr->infoMsg.DelPublishersByProc(it->first);
        this->dataPtr->infoSrv.DelPublishersByProc(it->first);

        // Notify without topic information. This is useful to inform the client
        // that a remote node is gone, even if we were not interested in its
        // topics.
        MessagePublisher publisher;
        publisher.SetPUuid(it->first);
        publisher.SetScope(Scope_t::ALL);
        this->dataPtr->disconnectionCb(publisher);

        // Remove the activity entry.
        this->dataPtr->activity.erase(it++);
      }
      else
        ++it;
    }
    this->dataPtr->mutex.unlock();

#ifdef _WIN32
    // We don't know why, but on Windows, during shutdown, when compiled
    // in Release, when loaded into MATLAB, the sleep_for() call hangs.
    Sleep(this->dataPtr->activityInterval);
#else
    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->dataPtr->activityInterval));
#endif

    // Is it time to exit?
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
      if (this->dataPtr->exit)
        timeToExit = true;
    }
  }
#ifdef _WIN32
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
  this->dataPtr->threadActivityExiting = true;
#endif
}

//////////////////////////////////////////////////
void Discovery::RunHeartbeatTask()
{
  bool timeToExit = false;
  while (!timeToExit)
  {
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

      std::string pUuid = this->dataPtr->pUuid;
      Publisher pub("", "", this->dataPtr->pUuid, "", Scope_t::ALL);
      this->SendMsg(HeartbeatType, pub);

      // Re-advertise topics that are advertised inside this process.
      std::map<std::string, std::vector<MessagePublisher>> msgNodes;
      this->dataPtr->infoMsg.PublishersByProc(pUuid, msgNodes);
      for (const auto &topic : msgNodes)
      {
        for (const auto &node : topic.second)
          this->SendMsg(AdvType, node);
      }

      // Re-advertise services that are advertised inside this process.
      std::map<std::string, std::vector<ServicePublisher>> srvNodes;
      this->dataPtr->infoSrv.PublishersByProc(pUuid, srvNodes);
      for (const auto &topic : srvNodes)
      {
        for (const auto &node : topic.second)
          this->SendMsg(AdvSrvType, node);
      }
    }

#ifdef _WIN32
    // We don't know why, but on Windows, during shutdown, when compiled
    // in Release, when loaded into MATLAB, the sleep_for() call hangs.
    Sleep(this->dataPtr->heartbeatInterval);
#else
    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->dataPtr->heartbeatInterval));
#endif

    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

      if (!this->dataPtr->initialized)
      {
        ++this->dataPtr->numHeartbeatsUninitialized;
        if (this->dataPtr->numHeartbeatsUninitialized == 2)
        {
          // We consider the discovery initialized after two cycles of
          // heartbeats sent.
          this->dataPtr->initialized = true;

          // Notify anyone waiting for the initialization phase to finish.
          this->dataPtr->initializedCv.notify_all();
        }
      }
    }

    // Is it time to exit?
    {
      std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
      if (this->dataPtr->exit)
        timeToExit = true;
    }
  }
#ifdef _WIN32
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
  this->dataPtr->threadHeartbeatExiting = true;
#endif
}

//////////////////////////////////////////////////
void Discovery::RunReceptionTask()
{
  bool timeToExit = false;
  while (!timeToExit)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {0, this->dataPtr->sockets.at(0), ZMQ_POLLIN, 0},
    };
    try
    {
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]),
        this->dataPtr->Timeout);
    }
    catch(...)
    {
      continue;
    }

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
        timeToExit = true;
    }
  }
#ifdef _WIN32
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->exitMutex);
  this->dataPtr->threadReceptionExiting = true;
#endif
}

//////////////////////////////////////////////////
void Discovery::RecvDiscoveryUpdate()
{
  char rcvStr[DiscoveryPrivate::MaxRcvStr];
  std::string srcAddr;
  uint16_t srcPort;
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);

  if ((recvfrom(this->dataPtr->sockets.at(0),
        reinterpret_cast<raw_type *>(rcvStr),
        DiscoveryPrivate::MaxRcvStr, 0, reinterpret_cast<sockaddr *>(&clntAddr),
        reinterpret_cast<socklen_t *>(&addrLen))) < 0)
  {
    std::cerr << "Discovery::RecvDiscoveryUpdate() recvfrom error" << std::endl;
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

  // Create the header from the raw bytes.
  header.Unpack(_msg);
  pBody += header.HeaderLength();

  // Discard the message if the wire protocol is different than mine.
  if (this->dataPtr->Version != header.Version())
    return;

  auto recvPUuid = header.PUuid();

  // Discard our own discovery messages.
  if (recvPUuid == this->dataPtr->pUuid)
    return;

  // Update timestamp and cache the callbacks.
  MsgDiscoveryCallback connectCb;
  MsgDiscoveryCallback disconnectCb;
  SrvDiscoveryCallback connectSrvCb;
  SrvDiscoveryCallback disconnectSrvCb;
  {
    std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
    this->dataPtr->activity[recvPUuid] = std::chrono::steady_clock::now();
    connectCb = this->dataPtr->connectionCb;
    disconnectCb = this->dataPtr->disconnectionCb;
    connectSrvCb = this->dataPtr->connectionSrvCb;
    disconnectSrvCb = this->dataPtr->disconnectionSrvCb;
  }

  switch (header.Type())
  {
    case AdvType:
    {
      // Read the rest of the fields.
      transport::AdvertiseMessage<MessagePublisher> advMsg;
      advMsg.Unpack(pBody);

      // Check scope of the topic.
      if ((advMsg.Publisher().Scope() == Scope_t::PROCESS) ||
          (advMsg.Publisher().Scope() == Scope_t::HOST &&
           _fromIp != this->dataPtr->hostAddr))
      {
        return;
      }

      // Register an advertised address for the topic.
      bool added;
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        added = this->dataPtr->infoMsg.AddPublisher(advMsg.Publisher());
      }

      if (added && connectCb)
      {
        // Execute the client's callback.
        connectCb(advMsg.Publisher());
      }

      break;
    }
    case AdvSrvType:
    {
      // Read the rest of the fields.
      transport::AdvertiseMessage<ServicePublisher> advSrv;
      advSrv.Unpack(pBody);

      // Check scope of the topic.
      if ((advSrv.Publisher().Scope() == Scope_t::PROCESS) ||
          (advSrv.Publisher().Scope() == Scope_t::HOST &&
           _fromIp != this->dataPtr->hostAddr))
      {
        return;
      }

      // Register an advertised address for the service.
      bool added;
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        added = this->dataPtr->infoSrv.AddPublisher(advSrv.Publisher());
      }

      if (added && connectSrvCb)
      {
        // Execute the client's callback.
        connectSrvCb(advSrv.Publisher());
      }

      break;
    }
    case SubType:
    {
      // Read the rest of the fields.
      SubscriptionMsg subMsg;
      subMsg.Unpack(pBody);
      auto recvTopic = subMsg.Topic();

      // Check if at least one of my nodes advertises the topic requested.
      MsgAddresses_M addresses;
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        if (!this->dataPtr->infoMsg.HasAnyPublishers(recvTopic,
              this->dataPtr->pUuid))
        {
          break;
        }

        if (!this->dataPtr->infoMsg.Publishers(recvTopic, addresses))
          break;
      }

      for (auto nodeInfo : addresses[this->dataPtr->pUuid])
      {
        // Check scope of the topic.
        if ((nodeInfo.Scope() == Scope_t::PROCESS) ||
            (nodeInfo.Scope() == Scope_t::HOST &&
             _fromIp != this->dataPtr->hostAddr))
        {
          continue;
        }

        // Answer an ADVERTISE message.
        this->SendMsg(AdvType, nodeInfo);
      }

      break;
    }
    case SubSrvType:
    {
      // Read the rest of the fields.
      SubscriptionMsg subMsg;
      subMsg.Unpack(pBody);
      auto recvTopic = subMsg.Topic();

      // Check if at least one of my nodes advertises the service requested.
      SrvAddresses_M addresses;
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        if (!this->dataPtr->infoSrv.HasAnyPublishers(
              recvTopic, this->dataPtr->pUuid))
        {
          break;
        }

        if (!this->dataPtr->infoSrv.Publishers(recvTopic, addresses))
          break;
      }

      for (auto nodeInfo : addresses[this->dataPtr->pUuid])
      {
        // Check scope of the topic.
        if ((nodeInfo.Scope() == Scope_t::PROCESS) ||
            (nodeInfo.Scope() == Scope_t::HOST &&
             _fromIp != this->dataPtr->hostAddr))
        {
          continue;
        }

        // Answer an ADVERTISE_SRV message.
        this->SendMsg(AdvSrvType, nodeInfo);
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
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        this->dataPtr->activity.erase(recvPUuid);
      }

      if (disconnectCb)
      {
        MessagePublisher msgPub;
        msgPub.SetPUuid(recvPUuid);
        msgPub.SetScope(Scope_t::ALL);
        // Notify the new disconnection.
        disconnectCb(msgPub);
      }

      if (disconnectSrvCb)
      {
        ServicePublisher srvPub;
        srvPub.SetPUuid(recvPUuid);
        srvPub.SetScope(Scope_t::ALL);
        // Notify the new disconnection.
        disconnectSrvCb(srvPub);
      }

      // Remove the address entry for this topic.
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        this->dataPtr->infoMsg.DelPublishersByProc(recvPUuid);
        this->dataPtr->infoSrv.DelPublishersByProc(recvPUuid);
      }

      break;
    }
    case UnadvType:
    {
      // Read the address.
      transport::AdvertiseMessage<MessagePublisher> advMsg;
      advMsg.Unpack(pBody);

      // Check scope of the topic.
      if ((advMsg.Publisher().Scope() == Scope_t::PROCESS) ||
          (advMsg.Publisher().Scope() == Scope_t::HOST &&
           _fromIp != this->dataPtr->hostAddr))
      {
        return;
      }

      if (disconnectCb)
      {
        // Notify the new disconnection.
        disconnectCb(advMsg.Publisher());
      }

      // Remove the address entry for this topic.
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        this->dataPtr->infoMsg.DelPublisherByNode(advMsg.Publisher().Topic(),
          advMsg.Publisher().PUuid(), advMsg.Publisher().NUuid());
      }

      break;
    }
    case UnadvSrvType:
    {
      // Read the address.
      transport::AdvertiseMessage<ServicePublisher> advSrv;
      advSrv.Unpack(pBody);

      // Check scope of the topic.
      if ((advSrv.Publisher().Scope() == Scope_t::PROCESS) ||
          (advSrv.Publisher().Scope() == Scope_t::HOST &&
           _fromIp != this->dataPtr->hostAddr))
      {
        return;
      }

      if (disconnectSrvCb)
      {
        // Notify the new disconnection.
        disconnectSrvCb(advSrv.Publisher());
      }

      // Remove the address entry for this topic.
      {
        std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
        this->dataPtr->infoSrv.DelPublisherByNode(advSrv.Publisher().Topic(),
          advSrv.Publisher().PUuid(), advSrv.Publisher().NUuid());
      }

      break;
    }
    default:
    {
      std::cerr << "Unknown message type [" << header.Type() << "]\n";
      break;
    }
  }
}

//////////////////////////////////////////////////
std::vector<int>& Discovery::Sockets() const
{
  return this->dataPtr->sockets;
}

//////////////////////////////////////////////////
sockaddr_in* Discovery::MulticastAddr() const
{
  return &this->dataPtr->mcastAddr;
}

//////////////////////////////////////////////////
bool Discovery::Verbose() const
{
  return this->dataPtr->verbose;
}

//////////////////////////////////////////////////
uint8_t Discovery::Version() const
{
  return this->dataPtr->Version;
}

//////////////////////////////////////////////////
void Discovery::PrintCurrentState() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);

  std::cout << "---------------" << std::endl;
  std::cout << std::boolalpha << "Enabled: "
            << this->dataPtr->enabled << std::endl;
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
void Discovery::TopicList(std::vector<std::string> &_topics) const
{
  this->WaitForInit();
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->infoMsg.TopicList(_topics);
}

//////////////////////////////////////////////////
void Discovery::ServiceList(std::vector<std::string> &_services) const
{
  this->WaitForInit();
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
  this->dataPtr->infoSrv.TopicList(_services);
}

//////////////////////////////////////////////////
std::recursive_mutex& Discovery::Mutex() const
{
  return this->dataPtr->mutex;
}

//////////////////////////////////////////////////
bool Discovery::RegisterNetIface(const std::string &_ip)
{
  // Make a new socket for sending discovery information.
  int sock = static_cast<int>(socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP));
  if (sock < 0)
  {
    std::cerr << "Socket creation failed." << std::endl;
    return false;
  }

  // Socket option: IP_MULTICAST_IF.
  // This socket option needs to be applied to each socket used to send data.
  // This option selects the source interface for outgoing messages.
  struct in_addr ifAddr;
  ifAddr.s_addr = inet_addr(_ip.c_str());
  if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
    reinterpret_cast<const char*>(&ifAddr), sizeof(ifAddr)) != 0)
  {
    std::cerr << "Error setting socket option (IP_MULTICAST_IF)." << std::endl;
    return false;
  }

  this->dataPtr->sockets.push_back(sock);

  // Join the multicast group. We have to do it for each network interface but
  // we can do it on the same socket. We will use the socket at position 0 for
  // receiving multicast information.
  struct ip_mreq group;
  group.imr_multiaddr.s_addr =
    inet_addr(this->dataPtr->MulticastGroup.c_str());
  group.imr_interface.s_addr = inet_addr(_ip.c_str());
  if (setsockopt(this->dataPtr->sockets.at(0), IPPROTO_IP, IP_ADD_MEMBERSHIP,
    reinterpret_cast<const char*>(&group), sizeof(group)) != 0)
  {
    std::cerr << "Error setting socket option (IP_ADD_MEMBERSHIP)."
              << std::endl;
    return false;
  }

  return true;
}

/////////////////////////////////////////////////
void Discovery::WaitForInit() const
{
  bool ready;
  {
    std::lock_guard<std::recursive_mutex> lock(this->dataPtr->mutex);
    ready = this->dataPtr->initialized;
  }

  if (!ready)
  {
    std::unique_lock<std::recursive_mutex> lk(this->dataPtr->mutex);
    this->dataPtr->initializedCv.wait(
       lk, [this]{return this->dataPtr->initialized;});
  }
}
