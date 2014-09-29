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

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <zmq.hpp>

class BasicResponser
{
  //////////////////////////////////////////////////
  public: BasicResponser()
    : context(new zmq::context_t(1)),
      responser(new zmq::socket_t(*context, ZMQ_DEALER))
  {
    int hwmVal = 0;
    int backlog = 100000;
    /*this->responser->setsockopt(ZMQ_SNDHWM, &hwmVal, sizeof(hwmVal));
    this->responser->setsockopt(ZMQ_RCVHWM, &hwmVal, sizeof(hwmVal));
    this->responser->setsockopt(ZMQ_BACKLOG, &backlog, sizeof(backlog));*/
    this->responser->bind("tcp://127.0.0.1:5555");

    std::cerr << "Bind" << std::endl;

    // Start the service thread.
    this->threadReception = new std::thread(&BasicResponser::RunReceptionTask,
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
        {*this->responser, 0, ZMQ_POLLIN, 0}
      };
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), 250);

      //  If we got a reply, process it.
      if (items[0].revents & ZMQ_POLLIN)
        this->RecvSrvRequest();
    }
  }

  //////////////////////////////////////////////////
  public: void RecvSrvRequest()
  {
    zmq::message_t msg(0);
    std::string request;
    std::string sender;
    std::string data = "response";
    std::string id;

    try
    {
      /*if (!this->responser->recv(&msg, 0))
        return;
      id = std::string(reinterpret_cast<char *>(msg.data()), msg.size());*/

      if (!this->responser->recv(&msg, 0))
        return;
      sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());
      if (!this->responser->recv(&msg, 0))
        return;
      request = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      std::cout << "Request received: " << request << std::endl;
      std::cout << "From: " << sender << std::endl;

      // zmq::context_t ctx(1);
      zmq::socket_t socket(*this->context, ZMQ_DEALER);
      int lingerVal = 200;
      int hwmVal = 0;
      int backlog = 100000;
      int numSockets = 100000;
      //socket.setsockopt(ZMQ_MAX_SOCKETS, &numSockets, sizeof(numSockets));
      //socket.setsockopt(ZMQ_SNDHWM, &hwmVal, sizeof(hwmVal));
      //socket.setsockopt(ZMQ_RCVHWM, &hwmVal, sizeof(hwmVal));
      //socket.setsockopt(ZMQ_BACKLOG, &backlog, sizeof(backlog));

      //static int counter = 0;
      //if (counter == 0)
        socket.connect(sender.c_str());
        std::cout << "Connect" << std::endl;
      //counter++;
      //std::cout << "Request #" << counter << std::endl;


      zmq::message_t response;
      response.rebuild(data.size());
      memcpy(response.data(), data.data(), data.size());
      assert(socket.send(response, 0) > 0);
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "RecvSrvResponse() error: " << _error.what() << std::endl;
      return;
    }
  }

  /// \brief 0MQ context.
  private: std::unique_ptr<zmq::context_t> context;

  /// \brief ZMQ socket for sending service call requests.
  private: std::unique_ptr<zmq::socket_t> responser;

  private: std::thread *threadReception;
};


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Will receive responses;
  BasicResponser responser;

  getchar();
}
