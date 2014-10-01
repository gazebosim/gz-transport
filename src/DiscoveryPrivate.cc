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

#include <uuid/uuid.h>
#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/NetUtils.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
DiscoveryPrivate::DiscoveryPrivate(const std::string &_pUuid, bool _verbose)
  : bcastAddr(BcastIP),
    pUuid(_pUuid),
    silenceInterval(DefSilenceInterval),
    activityInterval(DefActivityInterval),
    advertiseInterval(DefAdvertiseInterval),
    heartbeatInterval(DefHeartbeatInterval),
    connectionCb(nullptr),
    disconnectionCb(nullptr),
    verbose(_verbose),
    bcastSockIn(new UDPSocket(DiscoveryPort)),
    exit(false)
{
  // We need this environment variable in order to disable the signal capture
  // in czmq.
  putenv(const_cast<char*>("ZSYS_SIGHANDLER=true"));

  // Get this host IP address.
  this->hostAddr = DetermineHost();

  // Start the thread that receives discovery information.
  this->threadReception =
    new std::thread(&DiscoveryPrivate::RunReceptionTask, this);

  // Start the thread that sends heartbeats.
  this->threadHeartbeat =
    new std::thread(&DiscoveryPrivate::RunHeartbeatTask, this);

  // Start the thread that checks the topic information validity.
  this->threadActivity =
    new std::thread(&DiscoveryPrivate::RunActivityTask, this);

  if (this->verbose)
    this->PrintCurrentState();
}

//////////////////////////////////////////////////
DiscoveryPrivate::~DiscoveryPrivate()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service threads to finish before exit.
  this->threadReception->join();
  this->threadHeartbeat->join();
  this->threadActivity->join();

  // Broadcast a BYE message to trigger the remote cancellation of
  // all our advertised topics.
  this->SendMsg(ByeType, "", "", "", "", Scope::All);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Stop all the beacons.
  /*for (auto &topic : this->beacons)
  {
    for (auto &proc : topic.second)
    {
      zbeacon_t *b = proc.second;
      // Destroy the beacon.
      zbeacon_silence(b);
      zbeacon_destroy(&b);
    }
  }

  zbeacon_destroy(&this->beacon);
  zctx_destroy(&this->ctx);*/
}

//////////////////////////////////////////////////
void DiscoveryPrivate::Advertise(const MsgType &_advType,
  const std::string &_topic, const std::string &_addr, const std::string &_ctrl,
  const std::string &_nUuid, const Scope &_scope)
{
  std::lock_guard<std::mutex> lock(this->mutex);

  // Add the addressing information (local node).
  if (_advType == MsgType::Msg)
    this->infoMsg.AddAddress(_topic, _addr, _ctrl, this->pUuid, _nUuid, _scope);
  else
    this->infoSrv.AddAddress(_topic, _addr, _ctrl, this->pUuid, _nUuid, _scope);

  // If the scope is 'Process', do not advertise a message outside this process.
  if (_scope == Scope::Process)
    return;

  // Broadcast periodically my topic information.
  // this->NewBeacon(_advType, _topic, _nUuid);
  if (_advType == MsgType::Msg)
    this->SendMsg(AdvType, _topic, _addr, _ctrl, _nUuid, _scope);
  else
    this->SendMsg(AdvSrvType, _topic, _addr, _ctrl, _nUuid, _scope);
}

//////////////////////////////////////////////////
void DiscoveryPrivate::Unadvertise(const MsgType &_unadvType,
  const std::string &_topic, const std::string &_nUuid)
{
  std::lock_guard<std::mutex> lock(this->mutex);

  uint8_t msgType;
  TopicStorage *storage;

  if (_unadvType == MsgType::Msg)
  {
    msgType = UnadvType;
    storage = &this->infoMsg;
  }
  else
  {
    msgType = UnadvSrvType;
    storage = &this->infoSrv;
  }

  Address_t inf;
  // Don't do anything if the topic is not advertised by any of my nodes.
  if (!storage->GetAddress(_topic, this->pUuid, _nUuid, inf))
    return;

  // Remove the topic information.
  storage->DelAddressByNode(_topic, this->pUuid, _nUuid);

  // Do not advertise a message outside the process if the scope is 'Process'.
  if (inf.scope == Scope::Process)
    return;

  this->SendMsg(msgType, _topic, inf.addr, inf.ctrl, _nUuid, inf.scope);

  // Remove the beacon for this topic in this node.
  // this->DelBeacon(_topic, _nUuid);
}

//////////////////////////////////////////////////
void DiscoveryPrivate::Discover(const std::string &_topic, bool _isSrv)
{
  std::lock_guard<std::mutex> lock(this->mutex);

  uint8_t msgType;
  TopicStorage *storage;
  DiscoveryCallback cb;

  if (_isSrv)
  {
    msgType = SubSrvType;
    storage = &this->infoSrv;
    cb = this->connectionSrvCb;
  }
  else
  {
    msgType = SubType;
    storage = &this->infoMsg;
    cb = this->connectionCb;
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
void DiscoveryPrivate::RunActivityTask()
{
  while (true)
  {
    this->mutex.lock();

    Timestamp now = std::chrono::steady_clock::now();
    for (auto it = this->activity.cbegin(); it != this->activity.cend();)
    {
      // Elapsed time since the last update from this publisher.
      std::chrono::duration<double> elapsed = now - it->second;

      // This publisher has expired.
      if (std::chrono::duration_cast<std::chrono::milliseconds>
           (elapsed).count() > this->silenceInterval)
      {
        // Remove all the info entries for this process UUID.
        this->infoMsg.DelAddressesByProc(it->first);
        this->infoSrv.DelAddressesByProc(it->first);

        // Notify without topic information. This is useful to inform the client
        // that a remote node is gone, even if we were not interested in its
        // topics.
        this->disconnectionCb("", "", "", it->first, "", Scope::All);

        // Remove the activity entry.
        this->activity.erase(it++);
      }
      else
        ++it;
    }
    this->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->activityInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RunHeartbeatTask()
{
  while (true)
  {
    {
      std::lock_guard<std::mutex> lock(this->mutex);

      this->SendMsg(HeartbeatType, "", "", "", "", Scope::All);

      // Re-advertise topics that are advertised inside this process.
      std::map<std::string, std::vector<Address_t>> nodes;
      this->infoMsg.GetAddressesByProc(this->pUuid, nodes);
      for (auto &topic : nodes)
      {
        for (auto &node : topic.second)
        {
          this->SendMsg(AdvType, topic.first, node.addr, node.ctrl, node.nUuid,
            node.scope);
        }
      }

      // Re-advertise services that are advertised inside this process.
      this->infoSrv.GetAddressesByProc(this->pUuid, nodes);
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
      std::chrono::milliseconds(this->heartbeatInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RunReceptionTask()
{
  while (true)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {0, this->bcastSockIn->sockDesc, ZMQ_POLLIN, 0},
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->Timeout);

    //  If we got a reply, process it.
    if (items[0].revents & ZMQ_POLLIN)
    {
      this->RecvDiscoveryUpdate();

      if (this->verbose)
        this->PrintCurrentState();
    }

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RecvDiscoveryUpdate()
{
  char rcvStr[this->MaxRcvStr];
  std::string srcAddr;
  unsigned short srcPort;
  int bytes;

  try
  {
    bytes = this->bcastSockIn->recvFrom(rcvStr, this->MaxRcvStr, srcAddr,
                                        srcPort);
  }
  catch(const SocketException &_e)
  {
    std::cerr << "Exception receiving from the UDP discovery socket: "
              << _e.what() << std::endl;
    return;
  }

  if (this->verbose)
  {
    std::cout << "\nReceived discovery update from " << srcAddr << ": "
              << srcPort << "(" << bytes << " bytes)" << std::endl;
  }

  this->DispatchDiscoveryMsg(srcAddr, rcvStr);

  /*std::lock_guard<std::mutex> lock(this->mutex);

  // Address of datagram source.
  char *srcAddr = zstr_recv(zbeacon_socket(this->beacon));

  // A zmq message.
  zframe_t *frame = zframe_recv(zbeacon_socket(this->beacon));

  // Pointer to the raw discovery data.
  byte *data = zframe_data(frame);

  if (this->verbose)
    std::cout << "\nReceived discovery update from " << srcAddr << std::endl;

  this->DispatchDiscoveryMsg(std::string(srcAddr),
    reinterpret_cast<char*>(&data[0]));

  zstr_free(&srcAddr);
  zframe_destroy(&frame);*/
}

//////////////////////////////////////////////////
void DiscoveryPrivate::DispatchDiscoveryMsg(const std::string &_fromIp,
                                            char *_msg)
{
  Header header;
  char *pBody = _msg;

  std::lock_guard<std::mutex> lock(this->mutex);

  // Create the header from the raw bytes.
  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  auto recvPUuid = header.GetPUuid();

  // Discard our own discovery messages.
  if (recvPUuid == this->pUuid)
    return;

  // Update timestamp.
  this->activity[recvPUuid] = std::chrono::steady_clock::now();

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
          (recvScope == Scope::Host && _fromIp != this->hostAddr))
      {
        return;
      }

      DiscoveryCallback cb;
      TopicStorage *storage;

      if (header.GetType() == AdvType)
      {
        storage = &this->infoMsg;
        cb = this->connectionCb;
      }
      else
      {
        storage = &this->infoSrv;
        cb = this->connectionSrvCb;
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
        storage = &this->infoMsg;
      }
      else
      {
        msgType = AdvSrvType;
        storage = &this->infoSrv;
      }

      // Check if at least one of my nodes advertises the topic requested.
      if (storage->HasAnyAddresses(recvTopic, this->pUuid))
      {
        Addresses_M addresses;
        if (storage->GetAddresses(recvTopic, addresses))
        {
          for (auto nodeInfo : addresses[this->pUuid])
          {
            // Check scope of the topic.
            if ((nodeInfo.scope == Scope::Process) ||
                (nodeInfo.scope == Scope::Host && _fromIp != this->hostAddr))
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
      this->activity.erase(recvPUuid);

      if (this->disconnectionCb)
      {
        // Notify the new disconnection.
        this->disconnectionCb("", "", "", recvPUuid, "", Scope::All);
      }

      if (this->disconnectionSrvCb)
      {
        // Notify the new disconnection.
        this->disconnectionSrvCb("", "", "", recvPUuid, "", Scope::All);
      }

      // Remove the address entry for this topic.
      this->infoMsg.DelAddressesByProc(recvPUuid);
      this->infoSrv.DelAddressesByProc(recvPUuid);

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
          (recvScope == Scope::Host && _fromIp != this->hostAddr))
      {
        return;
      }

      DiscoveryCallback cb;
      TopicStorage *storage;

      if (header.GetType() == UnadvType)
      {
        storage = &this->infoMsg;
        cb = this->disconnectionCb;
      }
      else
      {
        storage = &this->infoSrv;
        cb = this->disconnectionSrvCb;
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
void DiscoveryPrivate::SendMsg(uint8_t _type, const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_nUuid,
  const Scope &_scope, int _flags)
{
  // Create the header.
  Header header(Version, this->pUuid, _type, _flags);
  size_t msgLength = 0;
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
      std::cerr << "DiscoveryPrivate::SendMsg() error: Unrecognized message"
                << " type [" << _type << "]" << std::endl;
      return;
  }

  // Broadcast the message.
  try
  {
    this->bcastSockOut.sendTo(reinterpret_cast<unsigned char*>(&buffer[0]),
      msgLength, this->bcastAddr, this->DiscoveryPort);
  }
  catch(const SocketException &_e)
  {
    std::cerr << "Exception sending a message: " << _e.what() << std::endl;
    return;
  }

  if (this->verbose)
  {
    std::cout << "\t* Sending " << MsgTypesStr[_type]
              << " msg [" << _topic << "]" << std::endl;
  }
}

//////////////////////////////////////////////////
std::string DiscoveryPrivate::GetHostAddr()
{
  return this->hostAddr;
}

//////////////////////////////////////////////////
void DiscoveryPrivate::PrintCurrentState()
{
  std::cout << "---------------" << std::endl;
  std::cout << "Discovery state" << std::endl;
  std::cout << "\tUUID: " << this->pUuid << std::endl;
  std::cout << "Settings" << std::endl;
  std::cout << "\tActivity: " << this->activityInterval << " ms." << std::endl;
  std::cout << "\tHeartbeat: " << this->heartbeatInterval << "ms." << std::endl;
  std::cout << "\tRetrans.: " << this->advertiseInterval << " ms."
    << std::endl;
  std::cout << "\tSilence: " << this->silenceInterval << " ms." << std::endl;
  std::cout << "Known msgs" << std::endl;
  this->infoMsg.Print();
  std::cout << "Known services" << std::endl;
  this->infoSrv.Print();

  // Used to calculate the elapsed time.
  Timestamp now = std::chrono::steady_clock::now();

  std::cout << "Activity" << std::endl;
  if (this->activity.empty())
    std::cout << "\t<empty>" << std::endl;
  else
  {
    for (auto &proc : this->activity)
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
/*void DiscoveryPrivate::NewBeacon(const MsgType &_advType,
  const std::string &_topic, const std::string &_nUuid)
{
  std::unique_ptr<Header> header;

  if (this->beacons.find(_topic) == this->beacons.end() ||
      this->beacons[_topic].find(_nUuid) == this->beacons[_topic].end())
  {
    // Create a new beacon and set the advertise interval.
    zbeacon_t *b = zbeacon_new(this->ctx, this->DiscoveryPort);
    zbeacon_set_interval(b, this->advertiseInterval);
    this->beacons[_topic][_nUuid] = b;

    Address_t node;

    // Create the header.
    if (_advType == MsgType::Msg)
    {
      // Prepare the content for the beacon.
      if (!this->infoMsg.GetAddress(_topic, this->pUuid, _nUuid, node))
        return;
      header.reset(new Header(Version, this->pUuid, AdvType));

      // Create the ADV message.
      AdvertiseMsg advMsg(*header, _topic, node.addr, node.ctrl, node.nUuid,
        node.scope, "not Used");
      std::vector<char> buffer(advMsg.GetMsgLength());
      advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

      // Setup the beacon.
      zbeacon_publish(
        b, reinterpret_cast<unsigned char*>(&buffer[0]), advMsg.GetMsgLength());
    }
    else
    {
      // Prepare the content for the beacon.
      if (!this->infoSrv.GetAddress(_topic, this->pUuid, _nUuid, node))
        return;
      header.reset(new Header(Version, this->pUuid, AdvSrvType));

      // Create the ADV SRV message.
      AdvertiseSrv advSrv(*header, _topic, node.addr, node.ctrl, node.nUuid,
        node.scope, "req not used", "rep not used");
      std::vector<char> buffer(advSrv.GetMsgLength());
      advSrv.Pack(reinterpret_cast<char*>(&buffer[0]));

      // Setup the beacon.
      zbeacon_publish(
        b, reinterpret_cast<unsigned char*>(&buffer[0]), advSrv.GetMsgLength());
    }
  }
}*/

//////////////////////////////////////////////////
/*void DiscoveryPrivate::DelBeacon(const std::string &_topic,
                                 const std::string &_nUuid)
{
  if (this->beacons.find(_topic) == this->beacons.end())
    return;

  if (this->beacons[_topic].find(_nUuid) == this->beacons[_topic].end())
    return;

  // Get the beacon.
  zbeacon_t *b = this->beacons[_topic][_nUuid];

  // Destroy the beacon.
  zbeacon_silence(b);
  zbeacon_destroy(&b);

  // Remove the beacon from the map.
  this->beacons[_topic].erase(_nUuid);
  if (this->beacons[_topic].empty())
    this->beacons.erase(_topic);
}*/
