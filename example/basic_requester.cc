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
    : context(new zmq::context_t(1)),
      requester(new zmq::socket_t(*context, ZMQ_DEALER))
  {
    int hwmVal = 0;
    int backlog = 100000;
    this->requester->setsockopt(ZMQ_BACKLOG, &backlog, sizeof(backlog));
    this->requester->setsockopt(ZMQ_SNDHWM, &hwmVal, sizeof(hwmVal));
    this->requester->setsockopt(ZMQ_RCVHWM, &hwmVal, sizeof(hwmVal));
    this->requester->bind("tcp://127.0.0.1:*");
    char bindEndPoint[1024];
    size_t size = sizeof(bindEndPoint);
    this->requester->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myAddress = bindEndPoint;
    std::cout << "Bind at " << this->myAddress << std::endl;

    // Start the service thread.
    this->threadReception = new std::thread(&BasicRequester::RunReceptionTask,
      this);
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
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), 250);

      std::cout << "poll" << std::endl;

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
    std::string sender;

    std::cout << "Response" << std::endl;

    try
    {
      /*if (!this->requester->recv(&msg, 0))
        return;
      sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      if (!this->requester->recv(&msg, 0))
        return;
      sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());*/

      if (!this->requester->recv(&msg, 0))
        return;
      this->response = std::string(reinterpret_cast<char *>(msg.data()),
        msg.size());
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "RecvSrvResponse() error: " << _error.what() << std::endl;
      return;
    }

    this->repAvailable = true;
    this->condition.notify_one();
  }

  //////////////////////////////////////////////////
  public: bool WaitUntilResponse()
  {
    auto now = std::chrono::system_clock::now();
    std::unique_lock<std::recursive_mutex> lk(this->mutex);
    return this->condition.wait_until(lk,
      now + std::chrono::milliseconds(1000),
      [this]
      {
        return this->repAvailable;
      });
  }

  /// \brief 0MQ context.
  private: std::unique_ptr<zmq::context_t> context;

  /// \brief ZMQ socket for sending service call requests.
  private: std::unique_ptr<zmq::socket_t> requester;

  public: std::string myAddress;

  private: std::thread *threadReception;

  public: std::string response = "empty";

  public: bool repAvailable = false;

  private: std::condition_variable_any condition;

  private: std::recursive_mutex mutex;
};


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Will receive responses;
  BasicRequester requester;

  while(true)
  {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_DEALER);
    int lingerVal = 200;
    int hwmVal = 0;
    int backlog = 100000;
    socket.setsockopt(ZMQ_SNDHWM, &hwmVal, sizeof(hwmVal));
    socket.setsockopt(ZMQ_RCVHWM, &hwmVal, sizeof(hwmVal));
    socket.setsockopt(ZMQ_BACKLOG, &backlog, sizeof(backlog));
    socket.connect("tcp://127.0.0.1:5555");

    try
    {
      zmq::message_t msg;
      std::string request = "request";

      std::cout << "Address size:" << requester.myAddress.size() << std::endl;

      msg.rebuild(requester.myAddress.size());
      memcpy(msg.data(), requester.myAddress.data(),
        requester.myAddress.size());
      assert(socket.send(msg, ZMQ_SNDMORE) > 0);

      msg.rebuild(request.size());
      memcpy(msg.data(), request.data(), request.size());
      assert(socket.send(msg, 0) > 0);

      std::cout << "Sending new message" << std::endl;

      // Wait for the response.
      if (requester.WaitUntilResponse())
        std::cout << "Response: " << requester.response << std::endl;
      else
        std::cout << "Time out" << std::endl;

      requester.response = "empty";
      requester.repAvailable = false;
    }
    catch(const zmq::error_t& ze)
    {
      std::cerr << "Error connecting [" << ze.what() << "]\n";
    }
  }
}
