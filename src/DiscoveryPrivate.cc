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

#include <czmq.h>
#include <uuid/uuid.h>
#include <zmq.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

//////////////////////////////////////////////////
transport::DiscoveryPrivate::DiscoveryPrivate(const uuid_t &_procUuid,
  bool _verbose)
  : silenceInterval(DefSilenceInterval),
    pollingInterval(DefPollingInterval),
    subInterval(DefSubInterval),
    helloInterval(DefHelloInterval),
    newDiscoveryEvent(nullptr),
    newDisconnectionEvent(nullptr),
    verbose(_verbose),
    exit(false)
{
  this->ctx = zctx_new();

  uuid_copy(this->procUuid, _procUuid);

  // Discovery beacon.
  this->beacon = zbeacon_new(this->ctx, this->DiscoveryPort);
  zbeacon_subscribe(this->beacon, NULL, 0);

  // Start the service thread.
  this->threadInbound =
    new std::thread(&transport::DiscoveryPrivate::Spin, this);

  // Start the service thread.
  this->threadHello =
     new std::thread(&transport::DiscoveryPrivate::SendHello, this);

  // Start the service thread.
  this->threadActivity =
    new std::thread(&transport::DiscoveryPrivate::UpdateActivity, this);

  // Start the service thread.
  this->threadSub =
    new std::thread(&transport::DiscoveryPrivate::RetransmitSubscriptions,
      this);
}

//////////////////////////////////////////////////
transport::DiscoveryPrivate::~DiscoveryPrivate()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service threads before exit.
  this->threadInbound->join();
  this->threadHello->join();
  this->threadActivity->join();
  this->threadSub->join();

  this->SendBye();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  zctx_destroy(&this->ctx);
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::SpinOnce()
{
  // Poll socket for a reply, with timeout.
  zmq::pollitem_t items[] = {
    { zbeacon_socket(this->beacon), 0, ZMQ_POLLIN, 0 }
  };
  zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->Timeout);

  //  If we got a reply, process it
  if (items[0].revents & ZMQ_POLLIN)
    this->RecvDiscoveryUpdate();
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::Spin()
{
  while (true)
  {
    this->SpinOnce();

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::UpdateActivity()
{
  while (true)
  {
    // Updating activity
    this->mutex.lock();

    transport::Timestamp now = std::chrono::steady_clock::now();
    for (auto it = this->activity.cbegin(); it != this->activity.cend();)
    {
      // Skip my own entry
      if (it->first == transport::GetGuidStr(this->procUuid))
        continue;

      std::chrono::duration<double> elapsed = now - it->second;
      if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
          > this->silenceInterval)
      {
        // Remove all the info entries for this proc UUID.
        for (auto it2 = this->info.cbegin(); it2 != this->info.cend();)
        {
          if (std::get<2>(it2->second) == it->first)
          {
            if (this->newDisconnectionEvent)
            {
              // Notify the new disconnection.
              transport::DiscTopicInfo topicInfo = it2->second;
              this->newDisconnectionEvent(it2->first, std::get<0>(topicInfo),
                std::get<1>(topicInfo), std::get<2>(topicInfo));
            }

            this->info.erase(it2++);
          }
          else
            ++it2;
        }

        // Remove the activity entry.
        this->activity.erase(it++);
      }
      else
        ++it;
    }
    this->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->pollingInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::SendHello()
{
  while (true)
  {
    this->mutex.lock();

    // Send a HELLO message.
    Header header(transport::Version, this->procUuid, "",
      transport::HelloType, 0);

    std::vector<char> buffer(header.GetHeaderLength());
    header.Pack(reinterpret_cast<char*>(&buffer[0]));

    // Just send one subscribe message.
    zbeacon_publish(this->beacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                    header.GetHeaderLength());
    zbeacon_silence(this->beacon);

    this->mutex.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(this->helloInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::SendBye()
{
  this->mutex.lock();

  // Send a BYE message.
  Header header(transport::Version, this->procUuid, "", transport::ByeType, 0);

  std::vector<char> buffer(header.GetHeaderLength());
  header.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Just send one subscribe message.
  zbeacon_publish(this->beacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                  header.GetHeaderLength());
  zbeacon_silence(this->beacon);

  this->mutex.unlock();
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::RetransmitSubscriptions()
{
  while (true)
  {
    this->mutex.lock();

    for (auto topic : this->unknownTopics)
      this->SendSubscribeMsg(transport::SubType, topic);

    this->mutex.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(this->subInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void transport::DiscoveryPrivate::RecvDiscoveryUpdate()
{
  std::lock_guard<std::mutex> lock(this->mutex);

  // Address of datagram source.
  char *srcAddr = zstr_recv(zbeacon_socket(this->beacon));

  // A zmq message.
  zframe_t *frame = zframe_recv(zbeacon_socket(this->beacon));

  // Pointer to the raw discovery data.
  byte *data = zframe_data(frame);

  if (this->verbose)
    std::cout << "\nReceived discovery update from " << srcAddr << std::endl;

  if (this->DispatchDiscoveryMsg(reinterpret_cast<char*>(&data[0])) != 0)
    std::cerr << "Something went wrong parsing a discovery message\n";

  zstr_free(&srcAddr);
  zframe_destroy(&frame);
}

//////////////////////////////////////////////////
int transport::DiscoveryPrivate::DispatchDiscoveryMsg(char *_msg)
{
  Header header;
  AdvMsg advMsg;
  std::string address;
  std::string controlAddress;
  char *pBody = _msg;

  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  std::string topic = header.GetTopic();
  std::string recvProcUuid = transport::GetGuidStr(header.GetGuid());

  // Discard our own discovery messages.
  if (recvProcUuid == transport::GetGuidStr(this->procUuid))
    return 0;

  // Update timestamp.
  this->activity[recvProcUuid] = std::chrono::steady_clock::now();

  switch (header.GetType())
  {
    case transport::AdvType:
      // Read the address
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();
      controlAddress = advMsg.GetControlAddress();

      if (this->verbose)
      {
        header.Print();
        advMsg.PrintBody();
      }

      // Register the advertised address for the topic.
      this->info[topic] =
        transport::DiscTopicInfo(address, controlAddress, recvProcUuid);

      // Remove topic from unkown topics.
      this->unknownTopics.erase(std::remove(this->unknownTopics.begin(),
        this->unknownTopics.end(), topic), this->unknownTopics.end());

      if (this->newDiscoveryEvent)
      {
        transport::DiscTopicInfo topicInfo = this->info[topic];
        this->newDiscoveryEvent(topic, std::get<0>(topicInfo),
          std::get<1>(topicInfo), std::get<2>(topicInfo));
      }
      break;

    case transport::SubType:
      // Check if I advertise the topic requested.
      if (this->AdvertisedByMe(topic))
      {
        // Send to the broadcast socket an ADVERTISE message.
        this->SendAdvertiseMsg(transport::AdvType, topic);
      }

      break;

    case transport::HelloType:
      // The timestamp has already been updated.
      break;

    case transport::ByeType:
      // The process is removed (disconnected).
      this->activity.erase(recvProcUuid);

      // Remove all the topic information that has the same proc UUID.
      for (auto it = this->info.cbegin(); it != this->info.cend();)
      {
        if (std::get<2>(it->second) == recvProcUuid)
        {
          if (this->newDisconnectionEvent)
          {
            // Notify the new disconnection.
            transport::DiscTopicInfo topicInfo = it->second;
            this->newDisconnectionEvent(it->first, std::get<0>(topicInfo),
              std::get<1>(topicInfo), std::get<2>(topicInfo));
          }

          this->info.erase(it++);
        }
        else
          ++it;
      }

      break;

    case transport::UnadvType:

      if (this->newDisconnectionEvent)
      {
        // Notify the new disconnection.
        transport::DiscTopicInfo topicInfo = this->info[topic];
        this->newDisconnectionEvent(topic, std::get<0>(topicInfo),
          std::get<1>(topicInfo), std::get<2>(topicInfo));
      }

      // Remove the topic info.
      this->info.erase(topic);

      break;

    default:
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
  }

  return 0;
}

//////////////////////////////////////////////////
int transport::DiscoveryPrivate::SendAdvertiseMsg(uint8_t _type,
                                           const std::string &_topic)
{
  assert(_topic != "");

  if (this->info.find(_topic) == this->info.end())
  {
    std::cerr << "SendAdvertiseMsg() Do not have information for topic ["
              << _topic << std::endl;
    return -1;
  }
  // Get the addresses associated to the topic.
  std::string addr = std::get<0>(this->info[_topic]);
  std::string ctrlAddr = std::get<1>(this->info[_topic]);

  if (this->verbose)
  {
    std::cout << "\t* Sending " << _type << " msg [" << _topic << "]["
              << addr << "][" << ctrlAddr << "]" << std::endl;
  }

  // Create the beacon content.
  Header header(transport::Version, this->procUuid, _topic, _type, 0);
  AdvMsg advMsg(header, addr, ctrlAddr);
  std::vector<char> buffer(advMsg.GetMsgLength());
  advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Just send one advertise message.
  zbeacon_publish(this->beacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                  advMsg.GetMsgLength());
  zbeacon_silence(this->beacon);

  return 0;
}

//////////////////////////////////////////////////
int transport::DiscoveryPrivate::SendSubscribeMsg(uint8_t _type,
                                           const std::string &_topic)
{
  assert(_topic != "");

  if (this->verbose)
    std::cout << "\t* Sending SUB msg [" << _topic << "]" << std::endl;

  Header header(transport::Version, this->procUuid, _topic, _type, 0);

  std::vector<char> buffer(header.GetHeaderLength());
  header.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Just send one subscribe message.
  zbeacon_publish(this->beacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                  header.GetHeaderLength());
  zbeacon_silence(this->beacon);

  return 0;
}

//////////////////////////////////////////////////
bool transport::DiscoveryPrivate::AdvertisedByMe(const std::string &_topic)
{
  if (this->info.find(_topic) == this->info.end())
    return false;

  return std::get<2>(this->info[_topic]) ==
    transport::GetGuidStr(this->procUuid);
}
