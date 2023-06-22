/*
 * Copyright (C) 2023 Open Source Robotics Foundation
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
#include <gz/transport/Node.hh>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <functional>
#include <tuple>
#include <utility>

namespace py = pybind11;

namespace gz
{
namespace transport
{
namespace python
{

PYBIND11_MODULE(BINDINGS_MODULE_NAME, m) {
    py::class_<AdvertiseOptions>(
      m, "AdvertiseOptions",
      "A class for customizing the publication options for"
      " a topic or service advertised")
      .def(py::init<>())
      .def_property("scope",
          &AdvertiseOptions::Scope,
          &AdvertiseOptions::SetScope,
          "The scope used in this topic/service");

    py::class_<AdvertiseMessageOptions, AdvertiseOptions>(
      m, "AdvertiseMessageOptions",
      "A class for customizing the publication options for a topic")
      .def(py::init<>())
      .def_property_readonly("throttled",
          &AdvertiseMessageOptions::Throttled,
          "Whether the publication has been throttled")
      .def_property("msgs_per_sec",
          &AdvertiseMessageOptions::MsgsPerSec,
          &AdvertiseMessageOptions::SetMsgsPerSec,
          "The maximum number of messages per second to be published");

    py::class_<SubscribeOptions>(
      m, "SubscribeOptions",
      "A class to provide different options for a subscription")
      .def(py::init<>())
      .def_property_readonly("throttled",
          &SubscribeOptions::Throttled,
          "Whether the subscription has been throttled")
      .def_property("msgs_per_sec",
          &SubscribeOptions::MsgsPerSec,
          &SubscribeOptions::SetMsgsPerSec,
          "Set the maximum number of messages per second received per topic");

    py::class_<MessageInfo>(
      m, "MessageInfo",
      "A class that provides information about the message received.")
      .def(py::init<>());

    auto node = py::class_<Node>(m, "Node",
      "A class that allows a client to communicate with other peers."
      " There are two main communication modes: pub/sub messages"
      " and service calls")
      .def(py::init<>())
      .def("advertise", static_cast<
          Node::Publisher (Node::*) (
              const std::string &,
              const std::string &,
              const AdvertiseMessageOptions &
          )>(&Node::Advertise),
          py::arg("topic"),
          py::arg("msg_type_name"),
          py::arg("options"),
          "Advertise a new topic. If a topic is currently advertised,"
          " you cannot advertise it a second time (regardless of its type)")
      .def("advertised_topics", &Node::AdvertisedTopics,
          "Get the list of topics advertised by this node")
      .def("subscribe", [](
          Node &_node,
          const std::string &_topic,
          std::function<void(const google::protobuf::Message &_msg)> &_callback,
          const SubscribeOptions &_opts)
          {
            return _node.Subscribe(_topic, _callback, _opts);
          },
          py::arg("topic"),
          py::arg("callback"),
          py::arg("options"),
          "Subscribe to a topic registering a callback")
      .def("subscribe_raw", [](
          Node &_node,
          const std::string &_topic,
          std::function<void(py::bytes _msgData, const size_t _size,
                           const MessageInfo &_info)> &_callback,
          const std::string &_msgType,
          const SubscribeOptions &_opts)
          {
            auto _cb = [_callback](const char *_msgData, const size_t _size,
                           const MessageInfo &_info){
                return _callback(py::bytes(_msgData, _size), _size, _info);
            };
            return _node.SubscribeRaw(_topic, _cb, _msgType, _opts);
          },
          py::arg("topic"),
          py::arg("callback"),
          py::arg("msg_type"),
          py::arg("options"))
      .def("subscribed_topics", &Node::SubscribedTopics,
          "Get the list of topics subscribed by this node")
      .def("unsubscribe", &Node::Unsubscribe,
          py::arg("topic"),
          "Unsubscribe from a topic")
      .def("topic_list", [](
          Node &_node)
          {
            std::vector<std::string> topics;
            _node.TopicList(topics);
            return topics;
          },
          "Get the list of topics currently advertised in the network")
      .def("topic_info", [](
          Node &_node,
          const std::string &_topic)
          {
            std::vector<MessagePublisher> publishers;
            std::vector<MessagePublisher> subscribers;
            _node.TopicInfo(_topic, publishers, subscribers);
            return std::make_pair(publishers, subscribers);
          },
          py::arg("topic"),
          "Get the information about a topic")
      .def("advertised_services", &Node::AdvertisedServices,
          "Get the list of services advertised by this node")
      // send a service request using the blocking interface
      .def("request_raw", [](
          Node &_node,
          const std::string &_service,
          const std::string &_request,
          const std::string &_reqType,
          const std::string &_repType,
          const unsigned int &_timeout,
          std::string &_response)
          {
            bool result{false};
            result = _node.RequestRaw(_service, _request, _reqType,
                            _repType, _timeout, _response, result);
            return std::make_tuple(result, py::bytes(_response.c_str(), _response.size()));
          },
          py::arg("topic"),
          py::arg("request"),
          py::arg("request_type"),
          py::arg("response_type"),
          py::arg("timeout"),
          py::arg("response"),
          "Request a new service without input parameter using"
          " a blocking call")
      .def("service_list", [](
          Node &_node)
          {
            std::vector<std::string> services;
            _node.ServiceList(services);
            return services;
          },
          "Get the list of topics currently advertised in the network")
      .def("service_info", [](
          Node &_node,
          const std::string &_service)
          {
            std::vector<ServicePublisher> publishers;
            _node.ServiceInfo(_service, publishers);
            return publishers;
          },
          py::arg("service"),
          "Get the information about a service");

  // register Node::Publisher as a subclass of Node
  py::class_<gz::transport::Node::Publisher>(node, "Publisher",
      "A class that is used to store information about an"
      " advertised publisher.")
      .def(py::init<>())
      .def(py::init<const gz::transport::Node::Publisher &>())
      .def("valid", &gz::transport::Node::Publisher::Valid,
          "Return true if valid information, such as a non-empty"
          " topic name, is present.")
      .def("publish_raw", &gz::transport::Node::Publisher::PublishRaw,
          py::arg("msgData"),
          py::arg("msgType"))
      .def("throttled_update_ready",
          &gz::transport::Node::Publisher::ThrottledUpdateReady,
          "")
      .def("has_connections",
          &gz::transport::Node::Publisher::HasConnections,
          "Return true if this publisher has subscribers");
}  // gz-transport13 module

}  // python
}  // transport
}  // gz
