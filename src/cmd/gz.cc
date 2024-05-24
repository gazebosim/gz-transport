/*
 * Copyright 2024 CogniPilot Foundation
 * Copyright 2024 Open Source Robotics Foundation
 * Copyright 2024 Rudis Laboratories
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

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
#include <google/protobuf/util/json_util.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <gz/msgs/Factory.hh>

#include "gz.hh"
#include "gz/transport/config.hh"
#include "gz/transport/Helpers.hh"
#include "gz/transport/Node.hh"

using namespace gz;
using namespace transport;

//////////////////////////////////////////////////
extern "C" void cmdTopicList()
{
  Node node;

  std::vector<std::string> topics;
  node.TopicList(topics);

  for (auto const &topic : topics)
    std::cout << topic << std::endl;
}

//////////////////////////////////////////////////
extern "C" void cmdTopicInfo(const char *_topic)
{
  if (!_topic || std::string(_topic).empty())
  {
    std::cerr << "Invalid topic. Topic must not be empty.\n";
    return;
  }

  // Get the publishers on the requested topic
  std::vector<MessagePublisher> publishers;
  std::vector<MessagePublisher> subscribers;
  Node node;
  node.TopicInfo(_topic, publishers, subscribers);

  if (!publishers.empty())
  {
    std::cout << "Publishers [Address, Message Type]:\n";

    // List the publishers
    for (std::vector<MessagePublisher>::iterator iter = publishers.begin();
        iter != publishers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
                << (*iter).MsgTypeName() << std::endl;
    }
  }
  else
  {
    std::cout << "No publishers on topic [" << _topic << "]\n";
  }

  // Get the subscribers on the requested topic
  if (!subscribers.empty())
  {
    std::cout << "Subscribers [Address, Message Type]:\n";

    // List the subscribers
    for (std::vector<MessagePublisher>::iterator iter = subscribers.begin();
        iter != subscribers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
                << (*iter).MsgTypeName() << std::endl;
    }
  }
  else
  {
    std::cout << "No subscribers on topic [" << _topic << "]\n";
  }
}

//////////////////////////////////////////////////
extern "C" void cmdServiceList()
{
  Node node;

  std::vector<std::string> services;
  node.ServiceList(services);

  for (auto const &service : services)
    std::cout << service << std::endl;
}

//////////////////////////////////////////////////
extern "C" void cmdServiceInfo(const char *_service)
{
  if (!_service || std::string(_service).empty())
  {
    std::cerr << "Invalid service. Service must not be empty.\n";
    return;
  }

  Node node;

  // Get the publishers on the requested topic
  std::vector<ServicePublisher> publishers;
  node.ServiceInfo(_service, publishers);

  if (!publishers.empty())
  {
    std::cout << "Service providers [Address, Request Message Type, "
              << "Response Message Type]:\n";

    /// List the publishers
    for (std::vector<ServicePublisher>::iterator iter = publishers.begin();
        iter != publishers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
        << (*iter).ReqTypeName() << ", " << (*iter).RepTypeName()
        << std::endl;
    }
  }
  else
  {
    std::cout << "No service providers on service [" << _service << "]\n";
  }
}

//////////////////////////////////////////////////
extern "C" void cmdTopicPub(const char *_topic,
  const char *_msgType, const char *_msgData)
{
  if (!_topic)
  {
    std::cerr << "Topic name is null\n";
    return;
  }

  if (!_msgType)
  {
    std::cerr << "Message type is null\n";
    return;
  }

  if (!_msgData)
  {
    std::cerr << "Message data is null\n";
    return;
  }

  // Create the message, and populate the field with _msgData
  auto msg = msgs::Factory::New(_msgType, _msgData);
  if (msg)
  {
    // Create the node and advertise the topic
    Node node;
    auto pub = node.Advertise(_topic, msg->GetTypeName());

    // Publish the message
    if (pub)
    {
      // \todo(anyone) Change this sleep to a WaitForSubscribers() call.
      // See issue #47.
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      pub.Publish(*msg);
    }
    else
    {
      std::cerr << "Unable to publish on topic[" << _topic << "] "
        << "with message type[" << _msgType << "].\n";
    }
  }
  else
  {
    std::cerr << "Unable to create message of type[" << _msgType << "] "
      << "with data[" << _msgData << "].\n";
  }
}

//////////////////////////////////////////////////
extern "C" void cmdServiceReq(const char *_service,
  const char *_reqType, const char *_repType, const int _timeout,
  const char *_reqData)
{
  if (!_service)
  {
    std::cerr << "Service name is null\n";
    return;
  }

  if (!_reqType)
  {
    std::cerr << "Request type is null\n";
    return;
  }

  if (!_repType)
  {
    std::cerr << "Response type is null\n";
    return;
  }

  if (!_reqData)
  {
    std::cerr << "Request data is null\n";
    return;
  }

  // Create the request, and populate the field with _reqData
  auto req = msgs::Factory::New(_reqType, _reqData);
  if (!req)
  {
    std::cerr << "Unable to create request of type[" << _reqType << "] "
              << "with data[" << _reqData << "].\n";
    return;
  }

  // Create the response.
  auto rep = msgs::Factory::New(_repType);
  if (!rep)
  {
    std::cerr << "Unable to create response of type[" << _repType << "].\n";
    return;
  }

  // Create the node.
  Node node;
  bool result;

  if (!strcmp(_repType, "gz.msgs.Empty"))
  {
    // One-way service.
    node.Request(_service, *req, 1000, *rep, result);
  }
  else
  {
    // Two-way service.
    bool executed = node.Request(_service, *req, _timeout, *rep, result);
    if (executed)
    {
      if (result)
        std::cout << rep->DebugString() << std::endl;
      else
        std::cout << "Service call failed" << std::endl;
    }
    else
      std::cerr << "Service call timed out" << std::endl;
  }
}

//////////////////////////////////////////////////
extern "C" void cmdTopicEcho(const char *_topic,
  const double _duration, int _count, MsgOutputFormat _outputFormat)
{
  if (!_topic || std::string(_topic).empty())
  {
    std::cerr << "Invalid topic. Topic must not be empty.\n";
    return;
  }

  std::mutex mutex;
  std::condition_variable condition;
  int count = 0;

  std::function<void(const ProtoMsg&)> cb = [&](const ProtoMsg &_msg)
  {
    std::lock_guard<std::mutex> lock(mutex);
    switch (_outputFormat)
    {
      case MsgOutputFormat::kDefault:
      case MsgOutputFormat::kDebugString:
        std::cout << _msg.DebugString() << std::endl;
        break;
      case MsgOutputFormat::kJSON:
        {
          std::string jsonStr;
          auto status =
              google::protobuf::util::MessageToJsonString(_msg, &jsonStr);
          if (status.ok())
          {
            std::cout << jsonStr << std::endl;
          }
          else
          {
            std::cerr << status;
          }
        }
        break;
      default:
        std::cerr << "Invalid output format selected.\n";
        return;
    }
    ++count;
    condition.notify_one();
  };

  Node node;
  if (!node.Subscribe(_topic, cb))
    return;

  if (_duration >= 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(
      static_cast<int64_t>(_duration * 1000)));
    return;
  }

  // Wait forever if _count <= 0. Otherwise wait for a specific number of
  // messages.
  if (_count <= 0)
  {
    waitForShutdown();
  }
  else
  {
    while (count < _count)
    {
      std::unique_lock<std::mutex> lock(mutex);
      condition.wait(lock, [&]{return count >= _count;});
    }
  }
}

//////////////////////////////////////////////////
extern "C" void cmdTopicFrequency(const char *_topic)
{
  if (!_topic || std::string(_topic).empty())
  {
    std::cerr << "Invalid topic. Topic must not be empty.\n";
    return;
  }
  using namespace std::chrono;
  std::mutex mutex;
  std::condition_variable condition;
  int count = 0;
  int64_t  time_array[11];
  float interval_array[10];
  float sum = 0.0;
  float dev = 0.0;
  float mean = 0.0;
  float stdDev = 0.0;
  float min = 0.0;
  float max = 0.0;

  std::function<void(const ProtoMsg&)> cb = [&](const ProtoMsg &_msg)
  {
    std::lock_guard<std::mutex> lock(mutex);

    if (count > 11 || count == 0)
    {
      count = 0;
      sum = 0.0;
      dev = 0.0;
    }
    if (count < 11)
    {
      time_point<system_clock> now = system_clock::now();
      duration<int64_t, std::nano> duration = now.time_since_epoch();
      time_array[count] = duration.count();
    }
    if  (count == 11)
    {
      for (int i = 0; i < 10; ++i)
      {
        interval_array[i] = static_cast<float>((time_array[i+1]
                              - time_array[i]) / 1e+9);
      }

      for (int i = 0; i < 10; ++i)
      {
        if (i == 0)
        {
          min = interval_array[i];
          max = interval_array[i];
        }
        if (i > 0)
        {
          if (min > interval_array[i])
          {
            min = interval_array[i];
          }
          if (max < interval_array[i])
          {
            max = interval_array[i];
          }
        }
      }

      for(int i = 0; i < 10; ++i) {
        sum += interval_array[i];
      }

      mean = sum / 10;

      for(int i = 0; i < 10; ++i) {
        dev += pow(interval_array[i] - mean, 2);
      }
      stdDev = sqrt(dev / 10);
      std::cout << "\n" << std::endl;
      for(int i = 0; i < 10; ++i) {
        std::cout << "interval [" << i << "]:    "
          << interval_array[i] << "s" << std::endl;
      }
      std::cout << "average rate: " << 1.0 / mean << std::endl;
      std::cout << "min: " << min << "s max: " << max
                << "s std dev: " << stdDev << "s window: 10"
                << std::endl;
      std::string jsonStr;
      auto status =
        google::protobuf::util::MessageToJsonString(_msg, &jsonStr);
    }
    ++count;
    condition.notify_one();
  };

  Node node;
  if (!node.Subscribe(_topic, cb))
    return;

  waitForShutdown();
}

//////////////////////////////////////////////////
extern "C" const char *gzVersion()
{
  return GZ_TRANSPORT_VERSION_FULL;
}
