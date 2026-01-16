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
public:
  Bridge() : ctx_(1), pub_(ctx_, ZMQ_PUB)
  {
    // Bind ZMQ to port 9872
    pub_.bind("tcp://*:9872");
    std::cout << "[Bridge] ZMQ broadcasting on tcp://*:9872" << std::endl;
  }

  void Run()
  {
    while (true)
    {
      UpdateSubscriptions();
      std::this_thread::sleep_for(2s);
    }
  }

private:
  gz::transport::Node node_;
  zmq::context_t ctx_;
  zmq::socket_t pub_;
  std::set<std::string> subscribed_topics_;
  std::map<std::string, std::string> topic_to_type_;
  google::protobuf::DynamicMessageFactory factory_;

  // The Generic Callback to convert and forward protobuf messages as JSON
  void OnMessage(const char *_msg_data, size_t _msg_len,
                 const gz::transport::MessageInfo &_info)
  {
    const auto &topic = _info.Topic();
    auto it = topic_to_type_.find(topic);
    if (it == topic_to_type_.end())
    {
      return;
    }
    const auto &msg_type = it->second;

    // Get descriptor from the generated pool
    const auto *pool = google::protobuf::DescriptorPool::generated_pool();
    const google::protobuf::Descriptor *descriptor =
        pool->FindMessageTypeByName(msg_type);
    if (!descriptor)
    {
      std::cerr << "OnMessage: Could not find descriptor for type: "
                << msg_type << std::endl;
      return;
    }

    // Create a dynamic message from the descriptor
    const google::protobuf::Message *prototype = factory_.GetPrototype(descriptor);
    if (!prototype)
    {
      std::cerr << "OnMessage: Could not get prototype for type: "
                << msg_type << std::endl;
      return;
    }

    std::unique_ptr<google::protobuf::Message> msg(prototype->New());
    if (!msg->ParseFromArray(_msg_data, _msg_len))
    {
      std::cerr << "OnMessage: Failed to parse message of type: "
                << msg_type << std::endl;
      return;
    }

    // Convert the message to a JSON string
    std::string msg_json_string;
    google::protobuf::json::PrintOptions options;
    options.always_print_fields_with_no_presence  = true;
    options.preserve_proto_field_names = true;
    // PlotJuggler does not support quoted integer values
    options.unquote_int64_if_possible = true;
    auto status = google::protobuf::util::MessageToJsonString(
        *msg, &msg_json_string, options);

    if (!status.ok())
    {
      std::cerr << "OnMessage: Failed to convert to JSON: "
                << status.ToString() << std::endl;
      return;
    }

    // Get a timestamp
    double timestamp = std::chrono::duration<double>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

    // Create the final JSON payload for PlotJuggler
    // Format: { "timestamp": 123.456, "values": { ... original message ... } }
    char final_json[msg_json_string.length() + 100];
    snprintf(final_json, sizeof(final_json),
             "{\"timestamp\":%f,\"values\":%s}",
             timestamp, msg_json_string.c_str());

    // if (topic == "/stats")
    // {
    //   std::cout << final_json << "\n";
    // }
    // Send a 2-part ZMQ message: [ topic_name, json_payload ]
    pub_.send(zmq::buffer(topic), zmq::send_flags::sndmore);
    pub_.send(zmq::buffer(std::string(final_json)));
  }

  void UpdateSubscriptions()
  {
    // Get list of all topics
    std::vector<std::string> topics;
    node_.TopicList(topics);

    for (const auto &topic : topics)
    {
      if (subscribed_topics_.count(topic) > 0)
        continue;

      // Get topic info
      std::vector<gz::transport::MessagePublisher> publishers;
      std::vector<gz::transport::MessagePublisher> subscribers;
      node_.TopicInfo(topic, publishers, subscribers);

      if (publishers.empty())
        continue;

      // Subscribe using the generic callback signature
      if (node_.SubscribeRaw(
              topic, std::bind(&Bridge::OnMessage, this, std::placeholders::_1,
                               std::placeholders::_2, std::placeholders::_3)))
      {
        const auto &msg_type = publishers[0].MsgTypeName();
        std::cout << "[+] Subscribed: " << topic << " [" << msg_type << "]"
                  << std::endl;
        subscribed_topics_.insert(topic);
        topic_to_type_[topic] = msg_type;
      }
    }
  }
};

int main()
{
  Bridge bridge;
  bridge.Run();
  return 0;
}

