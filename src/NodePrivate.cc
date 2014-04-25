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

#include <google/protobuf/message.h>
#include <uuid/uuid.h>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <zmq.hpp>
#include "ignition/transport/Node.hh"
#include "ignition/transport/NetUtils.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/socket.hh"
#include "ignition/transport/TopicsInfo.hh"
#include "ignition/transport/zmsg.hpp"

using namespace ignition;

//////////////////////////////////////////////////
transport::NodePrivate& transport::NodePrivate::GetInstance(bool _verbose)
{
  static NodePrivate *instance = new NodePrivate(_verbose);
  return *instance;
}

//////////////////////////////////////////////////
transport::NodePrivate::NodePrivate(bool _verbose)
  : bcastPort(11312),
    bcastSockIn(new UDPSocket(this->bcastPort)),
    bcastSockOut(new UDPSocket()),
    context(new zmq::context_t(1)),
    publisher(new zmq::socket_t(*context, ZMQ_PUB)),
    subscriber(new zmq::socket_t(*context, ZMQ_SUB))
{
  char bindEndPoint[1024];

  // Initialize random seed
  srand(time(nullptr));

  // Required 0MQ minimum version
  s_version_assert(2, 1);

  this->verbose = _verbose;

  // msecs
  this->timeout = 250;

  // ToDo Read this from getenv or command line arguments
  this->bcastAddr = "255.255.255.255";
  this->hostAddr = DetermineHost();

  uuid_generate(this->guid);

  this->guidStr = transport::GetGuidStr(this->guid);

  // 0MQ
  try
  {
    std::string anyTcpEP = "tcp://" + this->hostAddr + ":*";
    this->publisher->bind(anyTcpEP.c_str());
    size_t size = sizeof(bindEndPoint);
    this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->publisher->bind(InprocAddr.c_str());
    this->tcpEndpoint = bindEndPoint;
    this->myAddresses.push_back(this->tcpEndpoint);
    this->subscriber->connect(InprocAddr.c_str());
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "Error: " << ze.what() << std::endl;
     exit(EXIT_FAILURE);
  }

  if (this->verbose)
  {
    std::cout << "Current host address: " << this->hostAddr << std::endl;
    std::cout << "Bind at: [" << this->tcpEndpoint << "] for pub/sub\n";
    std::cout << "Bind at: [" << InprocAddr << "] for pub/sub\n";
    std::cout << "GUID: " << this->guidStr << std::endl;
  }

  this->threadInbound = new std::thread(&transport::NodePrivate::Spin, this);
}

//////////////////////////////////////////////////
transport::NodePrivate::~NodePrivate()
{
}

//////////////////////////////////////////////////
void transport::NodePrivate::SpinOnce()
{
  //  Poll socket for a reply, with timeout
  zmq::pollitem_t items[] = {
    { *this->subscriber, 0, ZMQ_POLLIN, 0 },
    { 0, this->bcastSockIn->sockDesc, ZMQ_POLLIN, 0 },
  };
  zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

  //  If we got a reply, process it
  if (items[0].revents & ZMQ_POLLIN)
    this->RecvTopicUpdates();
  else if (items[1].revents & ZMQ_POLLIN)
    this->RecvDiscoveryUpdates();
}

//////////////////////////////////////////////////
void transport::NodePrivate::Spin()
{
  while (true)
  {
    this->SpinOnce();
  }
}

//////////////////////////////////////////////////
int transport::NodePrivate::Publish(const std::string &_topic,
                                    const std::string &_data)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->mutex);

  if (this->topics.AdvertisedByMe(_topic))
  {
    zmsg msg;
    std::string sender = this->tcpEndpoint;
    msg.push_back((char*)_topic.c_str());
    msg.push_back((char*)sender.c_str());
    msg.push_back((char*)_data.c_str());

    if (this->verbose)
    {
      std::cout << "\nPublish(" << _topic << ")" << std::endl;
      msg.dump();
    }
    msg.send(*this->publisher);
    return 0;
  }
  else
  {
    if (this->verbose)
      std::cerr << "\nNot published. (" << _topic << ") not advertised\n";
    return -1;
  }
}

//////////////////////////////////////////////////
void transport::NodePrivate::RecvDiscoveryUpdates()
{
  char rcvStr[MaxRcvStr];     // Buffer for data
  std::string srcAddr;        // Address of datagram source
  unsigned short srcPort;     // Port of datagram source
  int bytes;                  // Rcvd from the UDP broadcast socket

  try
  {
    bytes = this->bcastSockIn->recvFrom(rcvStr, MaxRcvStr, srcAddr, srcPort);
  }
  catch(const SocketException &e)
  {
    cerr << "Exception receiving from the UDP socket: " << e.what() << endl;
    return;
  }

  if (this->verbose)
    cout << "\nReceived discovery update from " << srcAddr <<
            ": " << srcPort << " (" << bytes << " bytes)" << endl;

  if (this->DispatchDiscoveryMsg(rcvStr) != 0)
    std::cerr << "Something went wrong parsing a discovery message\n";
}

//////////////////////////////////////////////////
void transport::NodePrivate::RecvTopicUpdates()
{
  std::lock_guard<std::mutex> lock(this->mutex);

  zmsg *msg = new zmsg(*this->subscriber);
  if (this->verbose)
  {
    std::cout << "\nReceived topic update" << std::endl;
    msg->dump();
  }

  if (msg->parts() != 3)
  {
    std::cerr << "Unexpected topic update. Expected 3 message parts but "
              << "received a message with " << msg->parts() << std::endl;
    return;
  }

  // Read the DATA message
  std::string topic = std::string((char*)msg->pop_front().c_str());
  std::string sender = std::string((char*)msg->pop_front().c_str()); // Sender
  std::string data = std::string((char*)msg->pop_front().c_str());

  if (this->topics.Subscribed(topic))
  {
    // Execute the callback registered
    TopicInfo::Callback cb;
    if (this->topics.GetCallback(topic, cb))
      cb(topic, data);
    else
      std::cerr << "I don't have a callback for topic [" << topic << "]\n";
  }
  else
    std::cerr << "I am not subscribed to topic [" << topic << "]\n";
}

//////////////////////////////////////////////////
int transport::NodePrivate::DispatchDiscoveryMsg(char *_msg)
{
  std::lock_guard<std::mutex> lock(this->mutex);

  Header header;
  AdvMsg advMsg;
  std::string address;
  char *pBody = _msg;

  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  std::string topic = header.GetTopic();
  std::string rcvdGuid = transport::GetGuidStr(header.GetGuid());

  if (this->verbose)
    header.Print();

  switch (header.GetType())
  {
    case transport::AdvType:
      // Read the address
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();

      if (this->verbose)
        advMsg.PrintBody();

      // Register the advertised address for the topic
      this->topics.AddAdvAddress(topic, address);

      /*std::cout << "Subscribed? " << this->topics.Subscribed(topic) << std::endl;
      std::cout << "Connected? " << this->topics.Connected(topic) << std::endl;
      std::cout << "GUID? " << this->guidStr.compare(rcvdGuid) << std::endl;*/

      if (this->topics.Subscribed(topic))
      {
        // Add a filter for this topic
        this->subscriber->setsockopt(ZMQ_SUBSCRIBE, topic.data(), topic.size());
      }

      // Check if we are interested in this topic
      if (this->topics.Subscribed(topic) &&
          !this->topics.Connected(topic) &&
          this->guidStr.compare(rcvdGuid) != 0)
      {
        std::cout << "Connecting" << std::endl;
        try
        {
          this->subscriber->connect(address.c_str());
          this->topics.SetConnected(topic, true);
          if (this->verbose)
            std::cout << "\t* Connected to [" << address << "]\n";
        }
        catch(const zmq::error_t& ze)
        {
          std::cout << "Error connecting [" << ze.what() << "]\n";
        }
      }

      break;

    case transport::SubType:
      // Check if I advertise the topic requested
      if (this->topics.AdvertisedByMe(topic))
      {
        // Send to the broadcast socket an ADVERTISE message
        for (auto addr : this->myAddresses)
          this->SendAdvertiseMsg(transport::AdvType, topic, addr);
      }

      break;

    default:
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
  }

  return 0;
}

//////////////////////////////////////////////////
int transport::NodePrivate::SendAdvertiseMsg(uint8_t _type,
                                             const std::string &_topic,
                                             const std::string &_address)
{
  assert(_topic != "");

  if (this->verbose)
    std::cout << "\t* Sending ADV msg [" << _topic << "][" << _address
              << "]" << std::endl;

  Header header(transport::Version, this->guid, _topic, _type, 0);
  AdvMsg advMsg(header, _address);

  std::vector<char> buffer(advMsg.GetMsgLength());
  advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Send the data through the UDP broadcast socket
  try
  {
    this->bcastSockOut->sendTo(reinterpret_cast<char*>(&buffer[0]),
      advMsg.GetMsgLength(), this->bcastAddr, this->bcastPort);
  }
  catch(const SocketException &e)
  {
    cerr << "Exception sending an ADV msg: " << e.what() << endl;
    return -1;
  }

  return 0;
}

//////////////////////////////////////////////////
int transport::NodePrivate::SendSubscribeMsg(uint8_t _type,
                                             const std::string &_topic)
{
  assert(_topic != "");

  if (this->verbose)
    std::cout << "\t* Sending SUB msg [" << _topic << "]" << std::endl;

  Header header(transport::Version, this->guid, _topic, _type, 0);

  std::vector<char> buffer(header.GetHeaderLength());
  header.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Send the data through the UDP broadcast socket
  try
  {
    this->bcastSockOut->sendTo(reinterpret_cast<char*>(&buffer[0]),
      header.GetHeaderLength(), this->bcastAddr, this->bcastPort);
  }
  catch(const SocketException &e)
  {
    cerr << "Exception sending a SUB msg: " << e.what() << endl;
    return -1;
  }

  return 0;
}
