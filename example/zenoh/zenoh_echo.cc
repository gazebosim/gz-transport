/*
 * Copyright (C) 2025 Open Source Robotics Foundation
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

// Instructions:
// Terminal 1: GZ_PARTITION=zenoh GZ_TRANSPORT_IMPLEMENTATION=zenoh ./publisher
// Terminal 2:
//   . /opt/ros/rolling/setup.bash (if Zenoh is installed via zenoh_cpp_vendor)
//   ./zenoh_echo

//! [complete]
#include <google/protobuf/text_format.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <gz/msgs/Factory.hh>

#include <zenoh.hxx>

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Zenoh session.
  std::unique_ptr<zenoh::Session> session =
    std::make_unique<zenoh::Session>(
      zenoh::Session::open(zenoh::Config::create_default()));

  // Zenoh callback.
  auto dataHandler = [](const zenoh::Sample &_sample)
  {
    auto attachment = _sample.get_attachment();
    if (!attachment.has_value())
    {
      std::cerr << "Unable to find attachment. Ignoring message." << std::endl;
      return;
    }
    auto msgType = attachment->get().as_string();
    auto msgPtr = gz::msgs::Factory::New(msgType);
    if (!msgPtr)
      return;

    // Create the message using some serialized data.
    if (!msgPtr->ParseFromString(_sample.get_payload().as_string()))
    {
      std::cerr << "ParseFromString failed" << std::endl;
      return;
    }

    if (!msgPtr)
      return;

    if (std::string str;
        google::protobuf::TextFormat::PrintToString(*msgPtr, &str))
    {
      std::cout << str << std::endl;
    }
  };

  // Let's create a liveliness token containing some metainformation of the
  // subscriber. The token is used to construct the graph cache.
  std::string partition = "zenoh";
  std::string topic = "foo";
  std::string fullyQualifiedTopic = "@/" + partition + "@/" + topic;

  // Randomly generated UUIDs.
  std::string sessionId = "/dce0e931-41e9-480f-8910-67d42e36978c";
  std::string nodeId = "/1acf56d8-ae1f-4876-bbbe-577092a63c6e";
  std::string entityId = "/1acf56d8-ae1f-4876-bbbe-577092a63c6e";

  std::string entityKind = "/MS";
  std::string typeName = "/gz.msgs.StringMsg";

  std::string token = "@gz/%" + partition + sessionId + nodeId + entityId +
    entityKind + "/%/%/%/%" + topic + typeName + "/%/%";

  auto zSub = session->declare_subscriber(
    fullyQualifiedTopic, dataHandler, zenoh::closures::none);

  auto zToken = std::make_unique<zenoh::LivelinessToken>(
    session->liveliness_declare_token(token));

  // Zzzzzz.
  while (true)
    std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}
//! [complete]
