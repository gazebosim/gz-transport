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
#include <string>
#include <vector>
#include <zmq.hpp>
#include "ignition/transport/discZmq.hh"
#include "ignition/transport/netUtils.hh"
#include "ignition/transport/packet.hh"
#include "ignition/transport/socket.hh"
#include "ignition/transport/topicsInfo.hh"
#include "ignition/transport/zmsg.hpp"

using namespace ignition;

//////////////////////////////////////////////////
transport::Node::Node(const std::string &_master, bool _verbose, uuid_t *_guid)
{
  char bindEndPoint[1024];

  // Initialize random seed
  srand(time(nullptr));

  // Required 0MQ minimum version
  s_version_assert(2, 1);

  this->master = _master;
  this->verbose = _verbose;
  this->timeout = 250;           // msecs

  // ToDo Read this from getenv or command line arguments
  this->bcastAddr = "255.255.255.255";
  this->bcastPort = 11312;
  this->bcastSock = new UDPSocket(this->bcastPort);
  this->hostAddr = DetermineHost();

  if (_guid == nullptr)
    uuid_generate(this->guid);
  else
    uuid_copy(this->guid, *_guid);

  this->guidStr = transport::GetGuidStr(this->guid);

  // 0MQ
  try
  {
    this->context = new zmq::context_t(1);
    this->publisher = new zmq::socket_t(*this->context, ZMQ_PUB);
    this->subscriber = new zmq::socket_t(*this->context, ZMQ_SUB);
    this->srvRequester = new zmq::socket_t(*this->context, ZMQ_DEALER);
    this->srvReplier = new zmq::socket_t(*this->context, ZMQ_DEALER);
    std::string anyTcpEP = "tcp://" + this->hostAddr + ":*";
    this->publisher->bind(anyTcpEP.c_str());
    size_t size = sizeof(bindEndPoint);
    this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->publisher->bind(InprocAddr.c_str());
    this->tcpEndpoint = bindEndPoint;
    this->myAddresses.push_back(this->tcpEndpoint);
    this->subscriber->connect(InprocAddr.c_str());

    this->srvReplier->bind(anyTcpEP.c_str());
    this->srvReplier->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->srvReplierEP = bindEndPoint;
    this->mySrvAddresses.push_back(this->srvReplierEP);
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "Error: " << ze.what() << std::endl;
     this->Fini();
     exit(EXIT_FAILURE);
  }

  if (this->verbose)
  {
    std::cout << "Current host address: " << this->hostAddr << std::endl;
    std::cout << "Bind at: [" << this->tcpEndpoint << "] for pub/sub\n";
    std::cout << "Bind at: [" << InprocAddr << "] for pub/sub\n";
    std::cout << "Bind at: [" << this->srvReplierEP << "] for reps\n";
    std::cout << "Bind at: [" << this->srvRequesterEP << "] for reqs\n";
    std::cout << "GUID: " << this->guidStr << std::endl;
  }
}

//////////////////////////////////////////////////
transport::Node::~Node()
{
  this->Fini();
}

//////////////////////////////////////////////////
void transport::Node::SpinOnce()
{
  this->SendPendingAsyncSrvCalls();

  //  Poll socket for a reply, with timeout
  zmq::pollitem_t items[] = {
    { *this->subscriber, 0, ZMQ_POLLIN, 0 },
    { *this->srvReplier, 0, ZMQ_POLLIN, 0 },
    { 0, this->bcastSock->sockDesc, ZMQ_POLLIN, 0 },
    { *this->srvRequester, 0, ZMQ_POLLIN, 0 }
  };
  zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

  //  If we got a reply, process it
  if (items[0].revents & ZMQ_POLLIN)
    this->RecvTopicUpdates();
  else if (items[1].revents & ZMQ_POLLIN)
    this->RecvSrvRequest();
  else if (items[2].revents & ZMQ_POLLIN)
    this->RecvDiscoveryUpdates();
  else if (items[3].revents & ZMQ_POLLIN)
    this->RecvSrvReply();
}

//////////////////////////////////////////////////
void transport::Node::Spin()
{
  while (true)
  {
    this->SpinOnce();
  }
}

//////////////////////////////////////////////////
int transport::Node::Advertise(const std::string &_topic)
{
  assert(_topic != "");

  this->topics.SetAdvertisedByMe(_topic, true);

  for (auto it = this->myAddresses.begin(); it != this->myAddresses.end(); ++it)
    this->SendAdvertiseMsg(transport::AdvType, _topic, *it);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::UnAdvertise(const std::string &_topic)
{
  assert(_topic != "");

  this->topics.SetAdvertisedByMe(_topic, false);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::Publish(const std::string &_topic,
            const std::string &_data)
{
  assert(_topic != "");

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
int transport::Node::Publish(const std::string &_topic,
            const google::protobuf::Message &_message)
{
  assert(_topic != "");

  std::string data;
  _message.SerializeToString(&data);

  return this->Publish(_topic, data);
}

//////////////////////////////////////////////////
int transport::Node::Subscribe(const std::string &_topic,
  void(*_cb)(const std::string &, const std::string &))
{
  assert(_topic != "");
  if (this->verbose)
    std::cout << "\nSubscribe (" << _topic << ")\n";

  // Register our interest on the topic
  // The last subscribe call replaces previous subscriptions. If this is
  // a problem, we have to store a list of callbacks.
  this->topics.SetSubscribed(_topic, true);
  this->topics.SetCallback(_topic, _cb);

  // Add a filter for this topic
  this->subscriber->setsockopt(ZMQ_SUBSCRIBE, _topic.data(), _topic.size());

  // Discover the list of nodes that publish on the topic
  return this->SendSubscribeMsg(transport::SubType, _topic);
}

//////////////////////////////////////////////////
int transport::Node::UnSubscribe(const std::string &_topic)
{
  assert(_topic != "");
  if (this->verbose)
    std::cout << "\nUnubscribe (" << _topic << ")\n";

  this->topics.SetSubscribed(_topic, false);
  this->topics.SetCallback(_topic, nullptr);

  // Remove the filter for this topic
  this->subscriber->setsockopt(ZMQ_UNSUBSCRIBE, _topic.data(),
                              _topic.size());
  return 0;
}

//////////////////////////////////////////////////
int transport::Node::SrvAdvertise(const std::string &_topic,
  int(*_cb)(const std::string &, const std::string &, std::string &))
{
  assert(_topic != "");

  this->topicsSrvs.SetAdvertisedByMe(_topic, true);
  this->topicsSrvs.SetRepCallback(_topic, _cb);

  if (this->verbose)
    std::cout << "\nAdvertise srv call(" << _topic << ")\n";

  for (auto it = this->mySrvAddresses.begin();
       it != this->mySrvAddresses.end(); ++it)
    this->SendAdvertiseMsg(transport::AdvSvcType, _topic, *it);

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::SrvUnAdvertise(const std::string &_topic)
{
  assert(_topic != "");

  this->topicsSrvs.SetAdvertisedByMe(_topic, false);
  this->topicsSrvs.SetRepCallback(_topic, nullptr);

  if (this->verbose)
    std::cout << "\nUnadvertise srv call(" << _topic << ")\n";

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::SrvRequest(const std::string &_topic,
                                const std::string &_data,
                                std::string &_response)
{
  assert(_topic != "");

  this->topicsSrvs.SetRequested(_topic, true);

  int retries = 25;
  while (!this->topicsSrvs.Connected(_topic) && retries > 0)
  {
    this->SendSubscribeMsg(transport::SubSvcType, _topic);
    this->SpinOnce();
    s_sleep(200);
    --retries;
  }

  if (!this->topicsSrvs.Connected(_topic))
    return -1;

  // Send the request
  zmsg msg;
  msg.push_back((char*)_topic.c_str());
  msg.push_back((char*)this->tcpEndpoint.c_str());
  msg.push_back((char*)_data.c_str());

  if (this->verbose)
  {
    std::cout << "\nRequest (" << _topic << ")" << std::endl;
    msg.dump();
  }
  msg.send(*this->srvRequester);

  // Poll socket for a reply, with timeout
  zmq::pollitem_t items[] = { { *this->srvRequester, 0, ZMQ_POLLIN, 0 } };
  zmq::poll(items, 1, this->timeout);

  //  If we got a reply, process it
  if (items[0].revents & ZMQ_POLLIN)
  {
    zmsg *reply = new zmsg(*this->srvRequester);

    std::string((char*)reply->pop_front().c_str());
    std::string((char*)reply->pop_front().c_str());
    _response = std::string((char*)reply->pop_front().c_str());

    return 0;
  }

  return -1;
}

//////////////////////////////////////////////////
int transport::Node::SrvRequestAsync(const std::string &_topic,
                                     const std::string &_data,
  void(*_cb)(const std::string &_topic, int rc, const std::string &_rep))
{
  assert(_topic != "");

  this->topicsSrvs.SetRequested(_topic, true);
  this->topicsSrvs.SetReqCallback(_topic, _cb);
  this->topicsSrvs.AddReq(_topic, _data);

  if (this->verbose)
    std::cout << "\nAsync request (" << _topic << ")" << std::endl;

  this->SendSubscribeMsg(transport::SubSvcType, _topic);

  return 0;
}

//////////////////////////////////////////////////
void transport::Node::Fini()
{
  if (this->publisher) delete this->publisher;
  if (this->publisher) delete this->subscriber;
  if (this->publisher) delete this->srvRequester;
  if (this->publisher) delete this->srvReplier;
  if (this->publisher) delete this->context;

  this->myAddresses.clear();
  this->mySrvAddresses.clear();
}

//////////////////////////////////////////////////
void transport::Node::RecvDiscoveryUpdates()
{
  char rcvStr[MaxRcvStr];     // Buffer for data
  std::string srcAddr;        // Address of datagram source
  unsigned short srcPort;     // Port of datagram source
  int bytes;                  // Rcvd from the UDP broadcast socket

  try
  {
    bytes = this->bcastSock->recvFrom(rcvStr, MaxRcvStr, srcAddr, srcPort);
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
void transport::Node::RecvTopicUpdates()
{
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
  msg->pop_front(); // Sender
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
void transport::Node::RecvSrvRequest()
{
  zmsg *msg = new zmsg(*this->srvReplier);
  if (this->verbose)
  {
    std::cout << "\nReceived service request" << std::endl;
    msg->dump();
  }

  if (msg->parts() != 3)
  {
    std::cerr << "Unexpected service request. Expected 3 message parts but "
              << "received a message with " << msg->parts() << std::endl;
    return;
  }

  // Read the REQ message
  std::string topic = std::string((char*)msg->pop_front().c_str());
  std::string sender = std::string((char*)msg->pop_front().c_str());
  std::string data = std::string((char*)msg->pop_front().c_str());

  if (this->topicsSrvs.AdvertisedByMe(topic))
  {
    // Execute the callback registered
    TopicInfo::RepCallback cb;
    std::string response;
    if (this->topicsSrvs.GetRepCallback(topic, cb))
      cb(topic, data, response);
    else
      std::cerr << "I don't have a REP cback for topic [" << topic << "]\n";

    // Send the service call response
    zmsg reply;
    reply.push_back((char*)topic.c_str());
    reply.push_back((char*)this->srvReplierEP.c_str());
    reply.push_back((char*)response.c_str());
    // Todo: include the return code

    if (this->verbose)
    {
      std::cout << "\nResponse (" << topic << ")" << std::endl;
      reply.dump();
    }
    reply.send(*this->srvReplier);
  }
  else
  {
    std::cerr << "Received a svc call not advertised (" << topic << ")\n";
  }
}

//////////////////////////////////////////////////
void transport::Node::RecvSrvReply()
{
  zmsg *msg = new zmsg(*this->srvRequester);
  if (this->verbose)
  {
    std::cout << "\nReceived async service reply" << std::endl;
    msg->dump();
  }

  if (msg->parts() != 3)
  {
    std::cerr << "Unexpected service reply. Expected 3 message parts but "
              << "received a message with " << msg->parts() << std::endl;
    return;
  }

  // Read the SRV_REP message
  std::string topic = std::string((char*)msg->pop_front().c_str());
  std::string((char*)msg->pop_front().c_str());
  std::string response = std::string((char*)msg->pop_front().c_str());

  // Execute the callback registered
  TopicInfo::ReqCallback cb;
  if (this->topicsSrvs.GetReqCallback(topic, cb))
    // ToDo: send the return code
    cb(topic, 0, response);
  else
    std::cerr << "REQ callback for topic [" << topic << "] not found\n";
}

//////////////////////////////////////////////////
void transport::Node::SendPendingAsyncSrvCalls()
{
  // Check if there are any pending requests ready to send
  for (auto it = this->topicsSrvs.GetTopicsInfo().begin();
       it != this->topicsSrvs.GetTopicsInfo().end(); ++it)
  {
    std::string topic = it->first;

    if (!this->topicsSrvs.Connected(topic))
      continue;

    while (this->topicsSrvs.PendingReqs(topic))
    {
      std::string data;
      if (!this->topicsSrvs.DelReq(topic, data))
      {
        std::cerr << "Something went wrong removing a service request on "
                  << "topic (" << topic << ")\n";
        continue;
      }

      // Send the service call request
      zmsg msg;
      msg.push_back((char*)topic.c_str());
      msg.push_back((char*)this->srvRequesterEP.c_str());
      msg.push_back((char*)data.c_str());

      if (this->verbose)
      {
        std::cout << "\nAsync request [" << topic << "][" << data << "]\n";
        msg.dump();
      }
      msg.send(*this->srvRequester);
    }
  }
}

//////////////////////////////////////////////////
int transport::Node::DispatchDiscoveryMsg(char *_msg)
{
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

      std::cout << "subscribed? " << this->topics.Subscribed(topic) << std::endl;
      std::cout << "connected? " << this->topics.Connected(topic) << std::endl;
      std::cout << "GUID? " << this->guidStr.compare(rcvdGuid) << std::endl;

      // Check if we are interested in this topic
      if (this->topics.Subscribed(topic) &&
          !this->topics.Connected(topic) &&
          this->guidStr.compare(rcvdGuid) != 0)
      {
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

    case transport::AdvSvcType:
      // Read the address
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();

      if (this->verbose)
        advMsg.PrintBody();

      // Register the advertised address for the service call
      this->topicsSrvs.AddAdvAddress(topic, address);

      // Check if we are interested in this service call
      if (this->topicsSrvs.Requested(topic) &&
          !this->topicsSrvs.Connected(topic) &&
          this->guidStr.compare(rcvdGuid) != 0)
      {
        try
        {
          this->srvRequester->connect(address.c_str());
          this->topicsSrvs.SetConnected(topic, true);
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
        for (auto it = this->myAddresses.begin();
             it != this->myAddresses.end(); ++it)
          this->SendAdvertiseMsg(transport::AdvType, topic, *it);
      }

      break;

    case transport::SubSvcType:
      // Check if I advertise the service call requested
      if (this->topicsSrvs.AdvertisedByMe(topic))
      {
        // Send to the broadcast socket an ADV_SVC message
        for (auto it = this->mySrvAddresses.begin();
             it != this->mySrvAddresses.end(); ++it)
        {
          this->SendAdvertiseMsg(transport::AdvSvcType, topic, *it);
        }
      }

      break;

    default:
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
  }

  return 0;
}

//////////////////////////////////////////////////
int transport::Node::SendAdvertiseMsg(uint8_t _type, const std::string &_topic,
                                      const std::string &_address)
{
  assert(_topic != "");

  if (this->verbose)
    std::cout << "\t* Sending ADV msg [" << _topic << "][" << _address
              << "]" << std::endl;

  Header header(transport::Version, this->guid, _topic, _type, 0);
  AdvMsg advMsg(header, _address);

  char *buffer = new char[advMsg.GetMsgLength()];
  advMsg.Pack(buffer);

  // Send the data through the UDP broadcast socket
  try
  {
    this->bcastSock->sendTo(buffer, advMsg.GetMsgLength(),
      this->bcastAddr, this->bcastPort);
  }
  catch(const SocketException &e)
  {
    cerr << "Exception sending an ADV msg: " << e.what() << endl;
    delete[] buffer;
    return -1;
  }

  delete[] buffer;
  return 0;
}

//////////////////////////////////////////////////
int transport::Node::SendSubscribeMsg(uint8_t _type, const std::string &_topic)
{
  assert(_topic != "");

  if (this->verbose)
    std::cout << "\t* Sending SUB msg [" << _topic << "]" << std::endl;

  Header header(transport::Version, this->guid, _topic, _type, 0);

  char *buffer = new char[header.GetHeaderLength()];
  header.Pack(buffer);

  // Send the data through the UDP broadcast socket
  try
  {
    this->bcastSock->sendTo(buffer, header.GetHeaderLength(),
      this->bcastAddr, this->bcastPort);
  }
  catch(const SocketException &e)
  {
    cerr << "Exception sending a SUB msg: " << e.what() << endl;
    delete[] buffer;
    return -1;
  }

  delete[] buffer;
  return 0;
}
