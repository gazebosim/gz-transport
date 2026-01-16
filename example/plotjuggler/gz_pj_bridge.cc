#include <chrono>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <vector>

// Gazebo Includes
#include <gz/msgs/Factory.hh>
#include <gz/transport/Node.hh>

// Protobuf Includes
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

// ZeroMQ Includes
#include <zmq.hpp>

using namespace std::chrono_literals;

namespace
{
// Recursively add a FileDescriptor and its dependencies to a FileDescriptorSet
void AddFileDescriptor(const google::protobuf::FileDescriptor *file_desc,
                       google::protobuf::FileDescriptorSet *desc_set,
                       std::set<std::string> &visited_files)
{
  if (!file_desc)
  {
    return;
  }
  std::string file_name(file_desc->name());
  if (visited_files.count(file_name) > 0)
  {
    return;
  }
  visited_files.insert(file_name);

  // Add dependencies first
  for (int i = 0; i < file_desc->dependency_count(); ++i)
  {
    AddFileDescriptor(file_desc->dependency(i), desc_set, visited_files);
  }

  // Add the file descriptor itself
  google::protobuf::FileDescriptorProto *file_proto = desc_set->add_file();
  file_desc->CopyTo(file_proto);
}

// Get the serialized FileDescriptorSet for a given message type
std::string GetSchema(const std::string &msg_type)
{
  const auto *pool = google::protobuf::DescriptorPool::generated_pool();
  const google::protobuf::Descriptor *descriptor =
      pool->FindMessageTypeByName(msg_type);

  if (!descriptor)
  {
    std::cerr << "Could not find descriptor for type: " << msg_type << std::endl;
    return "";
  }

  google::protobuf::FileDescriptorSet desc_set;
  std::set<std::string> visited_files;
  AddFileDescriptor(descriptor->file(), &desc_set, visited_files);

  std::string schema_str;
  if (!desc_set.SerializeToString(&schema_str))
  {
    std::cerr << "Failed to serialize FileDescriptorSet" << std::endl;
    return "";
  }
  return schema_str;
}
}  // namespace

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

  // The Generic Callback to forward raw protobuf messages
  void OnMessage(const char *_msg_data, size_t _msg_len,
                 const gz::transport::MessageInfo &_info)
  {
    // 1. Get timestamp as a double
    double timestamp = std::chrono::duration<double>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

    // 2. Send multipart DATA message over ZMQ
    // Format: [ "DATA", topic_name, timestamp, raw_protobuf_bytes ]
    pub_.send(zmq::buffer("DATA"), zmq::send_flags::sndmore);
    pub_.send(zmq::buffer(_info.Topic()), zmq::send_flags::sndmore);
    pub_.send(zmq::buffer(&timestamp, sizeof(timestamp)),
              zmq::send_flags::sndmore);
    pub_.send(zmq::buffer(_msg_data, _msg_len));
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

        // Get schema and send it to PlotJuggler
        std::string schema = GetSchema(msg_type);
        if (!schema.empty())
        {
          // Send multipart SCHEMA message
          // Format: [ "SCHEMA", topic_name, type_name, serialized_schema ]
          pub_.send(zmq::buffer("SCHEMA"), zmq::send_flags::sndmore);
          pub_.send(zmq::buffer(topic), zmq::send_flags::sndmore);
          pub_.send(zmq::buffer(msg_type), zmq::send_flags::sndmore);
          pub_.send(zmq::buffer(schema));

          std::cout << "    -> Sent schema for [" << msg_type << "]"
                    << std::endl;
        }
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
