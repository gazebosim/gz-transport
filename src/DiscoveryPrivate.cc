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
using namespace transport;

//////////////////////////////////////////////////
DiscoveryPrivate::DiscoveryPrivate(const uuid_t &_procUuid,
  bool _verbose)
  : silenceInterval(DefSilenceInterval),
    activityInterval(DefActivityInterval),
    subscriptionInterval(DefSubscriptionInterval),
    helloInterval(DefHelloInterval),
    connectionCb(nullptr),
    disconnectionCb(nullptr),
    verbose(_verbose),
    exit(false)
{
  this->ctx = zctx_new();

  // Store the UUID and its string version.
  uuid_copy(this->uuid, _procUuid);
  this->uuidStr = GetGuidStr(this->uuid);

  // Discovery beacon.
  this->beacon = zbeacon_new(this->ctx, this->DiscoveryPort);
  zbeacon_subscribe(this->beacon, NULL, 0);

  // Start the thread that receives discovery information.
  this->threadInbound = new std::thread(&DiscoveryPrivate::Spin, this);

  // Start the thread that sends heartbeats.
  this->threadHello = new std::thread(&DiscoveryPrivate::SendHello, this);

  // Start the thread that checks the topic information validity.
  this->threadActivity =
    new std::thread(&DiscoveryPrivate::UpdateActivity, this);

  // Start the thread that retransmits the discovery requests.
  this->threadSub =
    new std::thread(&DiscoveryPrivate::RetransmitSubscriptions, this);
}

//////////////////////////////////////////////////
DiscoveryPrivate::~DiscoveryPrivate()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service threads to finish before exit.
  this->threadInbound->join();
  this->threadHello->join();
  this->threadActivity->join();
  this->threadSub->join();

  // Broadcast a BYE message to trigger the remote cancellation of
  // all our advertised topics.
  this->SendMsg(ByeType, "");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  zctx_destroy(&this->ctx);
}

//////////////////////////////////////////////////
void DiscoveryPrivate::Spin()
{
  while (true)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {zbeacon_socket(this->beacon), 0, ZMQ_POLLIN, 0}
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->Timeout);

    //  If we got a reply, process it.
    if (items[0].revents & ZMQ_POLLIN)
      this->RecvDiscoveryUpdate();

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::UpdateActivity()
{
  while (true)
  {
    this->mutex.lock();

    Timestamp now = std::chrono::steady_clock::now();
    for (auto it = this->activity.cbegin(); it != this->activity.cend();)
    {
      // Skip my own entry
      if (it->first == this->uuidStr)
        continue;

      // Elapsed time since the last update from this publisher.
      std::chrono::duration<double> elapsed = now - it->second;

      // This publisher has expired.
      if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
          > this->silenceInterval)
      {
        // Remove all the info entries for this process UUID.
        for (auto it2 = this->info.cbegin(); it2 != this->info.cend();)
        {
          // UUID matches
          if (std::get<Uuid>(it2->second) == it->first)
          {
            if (this->connectionCb)
            {
              // Notify the new disconnection.
              DiscoveryInfo topicInfo = it2->second;
              this->connectionCb(it2->first, std::get<Addr>(topicInfo),
                std::get<Ctrl>(topicInfo), std::get<Uuid>(topicInfo));
            }
            // Remove the info entry.
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
void DiscoveryPrivate::SendHello()
{
  while (true)
  {
    this->mutex.lock();
    this->SendMsg(HelloType, "");
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
void DiscoveryPrivate::RetransmitSubscriptions()
{
  while (true)
  {
    this->mutex.lock();

    // Iterate over the list of topics that have no discovery information yet.
    for (auto topic : this->unknownTopics)
      this->SendMsg(SubType, topic);

    this->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->subscriptionInterval));

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
int DiscoveryPrivate::DispatchDiscoveryMsg(char *_msg)
{
  Header header;
  AdvMsg advMsg;
  std::string address;
  std::string controlAddress;
  char *pBody = _msg;

  // Create the header from the raw bytes.
  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  std::string topic = header.GetTopic();
  std::string recvProcUuid = GetGuidStr(header.GetGuid());

  // Discard our own discovery messages.
  if (recvProcUuid == this->uuidStr)
    return 0;

  // Update timestamp.
  this->activity[recvProcUuid] = std::chrono::steady_clock::now();

  switch (header.GetType())
  {
    case AdvType:
      // Read the addresses.
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();
      controlAddress = advMsg.GetControlAddress();

      if (this->verbose)
      {
        header.Print();
        advMsg.PrintBody();
      }

      // Register the advertised address for the topic.
      this->info[topic] = DiscoveryInfo(address, controlAddress, recvProcUuid);

      // Remove topic from unkown topics.
      this->unknownTopics.erase(std::remove(this->unknownTopics.begin(),
        this->unknownTopics.end(), topic), this->unknownTopics.end());

      if (this->connectionCb)
      {
        // Execute the client's callback.
        DiscoveryInfo topicInfo = this->info[topic];
        this->connectionCb(topic, std::get<Addr>(topicInfo),
          std::get<Ctrl>(topicInfo), std::get<Uuid>(topicInfo));
      }
      break;

    case SubType:
      // Check if I advertise the topic requested.
      if (this->AdvertisedByMe(topic))
      {
        // Answer an ADVERTISE message.
        this->SendMsg(AdvType, topic);
      }

      break;

    case HelloType:
      // The timestamp has already been updated.
      break;

    case ByeType:
      // Remove the activity entry for this publisher.
      this->activity.erase(recvProcUuid);

      // Remove all the topic information that has the same process UUID.
      for (auto it = this->info.cbegin(); it != this->info.cend();)
      {
        // UUID matches.
        if (std::get<Uuid>(it->second) == recvProcUuid)
        {
          if (this->disconnectionCb)
          {
            // Notify the new disconnection.
            DiscoveryInfo topicInfo = it->second;
            this->disconnectionCb(it->first, std::get<Addr>(topicInfo),
              std::get<Ctrl>(topicInfo), std::get<Uuid>(topicInfo));
          }

          // Remove the info entry for this topic.
          this->info.erase(it++);
        }
        else
          ++it;
      }

      break;

    case UnadvType:

      if (this->disconnectionCb)
      {
        // Notify the new disconnection.
        DiscoveryInfo topicInfo = this->info[topic];
        this->disconnectionCb(topic, std::get<Addr>(topicInfo),
          std::get<Ctrl>(topicInfo), std::get<Uuid>(topicInfo));
      }

      // Remove the info entry for this topic.
      this->info.erase(topic);

      break;

    default:
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
  }

  return 0;
}

//////////////////////////////////////////////////
int DiscoveryPrivate::SendMsg(uint8_t _type, const std::string &_topic,
  int _flags)
{
  // Create the header.
  Header header(Version, this->uuid, _topic, _type, _flags);

  switch (_type)
  {
    case AdvType:
    {
      // Check if I have addressing information for this topic.
      if (this->info.find(_topic) == this->info.end())
        return -1;

      // Get the addresses associated to the topic.
      std::string addr = std::get<Addr>(this->info[_topic]);
      std::string ctrlAddr = std::get<Ctrl>(this->info[_topic]);

      // Create the ADVERTISE message.
      AdvMsg advMsg(header, addr, ctrlAddr);

      // Create a buffer and serialize the message.
      std::vector<char> buffer(advMsg.GetMsgLength());
      advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

      // Broadcast the message.
      zbeacon_publish(this->beacon,
        reinterpret_cast<unsigned char*>(&buffer[0]), advMsg.GetMsgLength());

      break;
    }
    case SubType:
    case UnadvType:
    case HelloType:
    case ByeType:
    {
      // Create a buffer and serialize the message.
      std::vector<char> buffer(header.GetHeaderLength());
      header.Pack(reinterpret_cast<char*>(&buffer[0]));

      // Broadcast the message.
      zbeacon_publish(this->beacon,
        reinterpret_cast<unsigned char*>(&buffer[0]), header.GetHeaderLength());

      break;
    }
    default:
      break;
  }

  zbeacon_silence(this->beacon);

  if (this->verbose)
  {
    std::cout << "\t* Sending " << MsgTypesStr[_type] << " msg [" << _topic
              << "]" << std::endl;
  }

  return 0;
}

//////////////////////////////////////////////////
bool DiscoveryPrivate::AdvertisedByMe(const std::string &_topic)
{
  if (this->info.find(_topic) == this->info.end())
    return false;

  return std::get<Uuid>(this->info[_topic]) == this->uuidStr;
}
