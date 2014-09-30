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
#include <string>
#include <thread>
#include <zmq.hpp>

class BasicResponser
{
  //////////////////////////////////////////////////
  public: BasicResponser()
  {
    this->context = new zmq::context_t(1);
    this->responser = new zmq::socket_t(*this->context, ZMQ_ROUTER);
    std::string ep = "tcp://127.0.0.1:5555";
    this->responser->setsockopt(ZMQ_IDENTITY, ep.c_str(), ep.size());
    this->responser->bind(ep.c_str());

    //this->replier = new zmq::socket_t(*this->context, ZMQ_DEALER);
    //this->replier->connect("tcp://127.0.0.1:6666");
    this->responser->connect("tcp://127.0.0.1:6666");

    // Start the reception thread.
    this->threadRcv = new std::thread(&BasicResponser::RunReceptionTask, this);
  }

  //////////////////////////////////////////////////
  public: ~BasicResponser()
  {
    delete this->responser;
    delete this->replier;
    delete this->context;
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
      zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->Timeout);

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
    std::string answer = "response";
    std::string id;

    try
    {
      if (!this->responser->recv(&msg, 0))
        return;
      id = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      if (!this->responser->recv(&msg, 0))
        return;
      sender = std::string(reinterpret_cast<char *>(msg.data()), msg.size());


      if (!this->responser->recv(&msg, 0))
        return;
      request = std::string(reinterpret_cast<char *>(msg.data()), msg.size());

      std::cout << "Id:" << id << std::endl;
      std::cout << "From: " << sender << std::endl;
      std::cout << "Request received: " << request << std::endl;

      zmq::message_t responseMsg;

      responseMsg.rebuild(sender.size());
      memcpy(responseMsg.data(), sender.data(), sender.size());
      assert(this->responser->send(responseMsg, ZMQ_SNDMORE) > 0);

      responseMsg.rebuild(answer.size());
      memcpy(responseMsg.data(), answer.data(), answer.size());
      assert(this->responser->send(responseMsg, 0) > 0);
    }
    catch(const zmq::error_t &_error)
    {
      std::cerr << "RecvSrvResponse() error: " << _error.what() << std::endl;
      return;
    }
  }

  /// \brief 0MQ context.
  private: zmq::context_t *context;

  /// \brief ZMQ socket for receiving service call requests.
  private: zmq::socket_t *responser;

  private: zmq::socket_t *replier;

  /// \brief Thread for receiving requests in a different thread.
  private: std::thread *threadRcv;

  /// \brief Polling interval.
  private: int Timeout = 250;
};


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  BasicResponser responser;
  getchar();
}
