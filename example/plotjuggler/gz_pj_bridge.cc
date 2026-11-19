/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

// Gazebo Includes
#include <gz/transport/Node.hh>

// Protobuf Includes
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>

// ZeroMQ Includes
#include <zmq.hpp>

using namespace std::chrono_literals;

class Bridge
{
  public: Bridge()
    : ctx(1),
      pub(ctx, ZMQ_PUB)
  {
    // Bind ZMQ to port 9872
    this->pub.bind("tcp://*:9872");
    std::cout << "[Bridge] ZMQ broadcasting on tcp://*:9872" << std::endl;
  }

  public: void Run()
  {
    while (true)
    {
      this->UpdateSubscriptions();
      std::this_thread::sleep_for(2s);
    }
  }

  private: void OnMessage(const char * _msg_data, size_t _msg_len,
                   const gz::transport::MessageInfo & _info)
  {
    const auto & topic = _info.Topic();
    auto it = this->topicToType.find(topic);
    if (it == this->topicToType.end())
    {
      return;
    }
    const auto & msgType = it->second;

    // Get descriptor from the generated pool
    const auto * pool = google::protobuf::DescriptorPool::generated_pool();
    const google::protobuf::Descriptor * descriptor =
        pool->FindMessageTypeByName(msgType);
    if (!descriptor)
    {
      std::cerr << "OnMessage: Could not find descriptor for type: " << msgType
                << std::endl;
      return;
    }

    // Create a dynamic message from the descriptor
    const google::protobuf::Message * prototype =
        this->factory.GetPrototype(descriptor);
    if (!prototype)
    {
      std::cerr << "OnMessage: Could not get prototype for type: " << msgType
                << std::endl;
      return;
    }

    std::unique_ptr<google::protobuf::Message> msg(prototype->New());
    if (!msg->ParseFromArray(_msg_data, _msg_len))
    {
      std::cerr << "OnMessage: Failed to parse message of type: " << msgType
                << std::endl;
      return;
    }

    // Convert the message to a JSON string
    std::string msgJsonString;
    google::protobuf::json::PrintOptions options;
    options.always_print_fields_with_no_presence = true;
    options.preserve_proto_field_names = true;
    // PlotJuggler does not support quoted integer values
    options.unquote_int64_if_possible = true;
    auto status = google::protobuf::util::MessageToJsonString(
        *msg, &msgJsonString, options);

    if (!status.ok())
    {
      std::cerr << "OnMessage: Failed to convert to JSON: " << status.ToString()
                << std::endl;
      return;
    }

    // Get a timestamp
    double timestamp = std::chrono::duration<double>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

    // Create the final JSON payload for PlotJuggler
    // Format: { "timestamp": 123.456, "values": { ... original message ... } }

    // if (topic == "/stats")
    // {
    //   std::cout << msgJsonString << "\n";
    // }
    // Send a 2-part ZMQ message: [ topic_name, json_payload ]
    this->pub.send(zmq::buffer(topic), zmq::send_flags::sndmore);
    this->pub.send(zmq::buffer(msgJsonString));
  }

  private: void UpdateSubscriptions()
  {
    // Get list of all topics
    std::vector<std::string> topics;
    this->node.TopicList(topics);

    for (const auto & topic : topics)
    {
      if (this->subscribedTopics.count(topic) > 0)
      {
        continue;
      }

      // Get topic info
      std::vector<gz::transport::MessagePublisher> publishers;
      std::vector<gz::transport::MessagePublisher> subscribers;
      this->node.TopicInfo(topic, publishers, subscribers);

      if (publishers.empty())
      {
        continue;
      }

      // Subscribe using the generic callback signature
      if (this->node.SubscribeRaw(
              topic, std::bind(&Bridge::OnMessage, this, std::placeholders::_1,
                               std::placeholders::_2, std::placeholders::_3)))
      {
        const auto & msgType = publishers[0].MsgTypeName();
        std::cout << "[+] Subscribed: " << topic << " [" << msgType << "]"
                  << std::endl;
        this->subscribedTopics.insert(topic);
        this->topicToType[topic] = msgType;
      }
    }
  }

  private: gz::transport::Node node;
  private: zmq::context_t ctx;
  private: zmq::socket_t pub;
  private: std::set<std::string> subscribedTopics;
  private: std::map<std::string, std::string> topicToType;
  private: google::protobuf::DynamicMessageFactory factory;
};

int main()
{
  Bridge bridge;
  bridge.Run();
  return 0;
}

