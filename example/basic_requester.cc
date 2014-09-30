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

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <zmq.hpp>

class BasicRequester
{
  //////////////////////////////////////////////////
  public: BasicRequester()
  {
    this->context = new zmq::context_t(1);
    this->requester = new zmq::socket_t(*context, ZMQ_ROUTER);

    // Set the identity.
    this->ep = "tcp://127.0.0.1:6666";
    this->requester->setsockopt(ZMQ_IDENTITY, "RequesterID", 11);

    this->requester->bind(this->ep.c_str());
    char bindEndPoint[1024];
    size_t size = sizeof(bindEndPoint);
    this->requester->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myAddress = bindEndPoint;
    std::cout << "Bind at " << this->myAddress << std::endl;

    // Start the service thread.
    this->threadRcv = new std::thread(&BasicRequester::RunReceptionTask, this);
  }

  //////////////////////////////////////////////////
  public: ~BasicRequester()
  {
    delete requester;
    delete context;
  }

  //////////////////////////////////////////////////
  public: void RunReceptionTask()
  {
    while (true)
    {
      // Poll socket for a reply, with timeout.
      zmq::pollitem_t items[] =
      {
        {*this->requester, 0, ZMQ_POLLIN, 0}
      };
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->PollTimeout);

      //  If we got a reply, process it.
      if (items[0].revents & ZMQ_POLLIN)
        this->RecvSrvResponse();
    }
  }

  //////////////////////////////////////////////////
  public: void RecvSrvResponse()
  {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    zmq::message_t msg(0);

    // Read the response.
    try
    {
      if (!this->requester->recv(&msg, 0))
        return;
      //id = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      if (!this->requester->recv(&msg, 0))
        return;
      this->response =
        std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      // Set the 'repAvailable' flag and notify to wake up the thread waiting for
      // the response.
      this->repAvailable = true;
      this->condition.notify_one();
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "RecvSrvResponse() error: " << _error.what() << std::endl;
      return;
    }
  }

  //////////////////////////////////////////////////
  public: bool WaitUntilResponse()
  {
    auto now = std::chrono::system_clock::now();
    std::unique_lock<std::recursive_mutex> lk(this->mutex);
    return this->condition.wait_until(lk,
      now + std::chrono::milliseconds(this->ReqTimeout),
      [this]
      {
        return this->repAvailable;
      });
  }

  /// \brief 0MQ context.
  private: zmq::context_t *context;

  /// \brief ZMQ socket for receiving service call responses.
  private: zmq::socket_t *requester;

  /// \brief My zeroMQ end point.
  public: std::string myAddress;

  /// \brief Thread for managing the service call responses.
  private: std::thread *threadRcv;

  /// \brief Received response.
  public: std::string response = "empty";

  public: std::string ep;

  /// \brief Flag that tells you when a new response is available.
  public: bool repAvailable = false;

  /// \brief Condition vaiable that prevents the requester from making a new
  /// request until the current response is received (or timed out)
  private: std::condition_variable_any condition;

  /// \brief Mutex used between the thread that makes the new requests and the
  /// thread that receives the responses.
  private: std::recursive_mutex mutex;

  /// \brief Polling interval (ms).
  private: int PollTimeout = 250;

  /// \brief Timeout used to wait for a response (ms).
  private: int ReqTimeout = 1000;
};


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Will receive responses in a separate thread;
  BasicRequester requester;

  // Context for this thread.
  zmq::context_t context(1);

  // Create a socket and connect it to the service provider.
  zmq::socket_t socket(context, ZMQ_ROUTER);
  std::string destinationEP = "tcp://127.0.0.1:5555";
  socket.setsockopt(ZMQ_IDENTITY, "SocketID", 8);
  socket.connect(destinationEP.c_str());

  // Send new requests after receiving a response.
  while(true)
  {
    try
    {
      // No response available yet.
      requester.response = "empty";
      requester.repAvailable = false;

      // Simulate a request.
      zmq::message_t msg;
      std::string request = "request";

       // Fill the message with the destination address.
      msg.rebuild(destinationEP.size());
      memcpy(msg.data(), destinationEP.data(),
        destinationEP.size());
      assert(socket.send(msg, ZMQ_SNDMORE) > 0);

      // Fill the message with my response address.
      std::string requesterId = "RequesterID";
      msg.rebuild(requesterId.size());
      memcpy(msg.data(), requesterId.data(), requesterId.size());
      assert(socket.send(msg, ZMQ_SNDMORE) > 0);

      // Fill the message with my request.
      msg.rebuild(request.size());
      memcpy(msg.data(), request.data(), request.size());
      assert(socket.send(msg, 0) > 0);

      std::cout << "Sending new message" << std::endl;

      // Wait for the response.
      if (requester.WaitUntilResponse())
        std::cout << "Response: " << requester.response << std::endl;
      else
        std::cout << "Time out" << std::endl;
    }
    catch(const zmq::error_t& ze)
    {
      std::cerr << "Error: [" << ze.what() << "]" << std::endl;
    }
  }
}
