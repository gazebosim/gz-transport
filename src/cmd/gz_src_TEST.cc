/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include <google/protobuf/text_format.h>
#include "gtest/gtest.h"
#include <gz/msgs/int32.pb.h>
#include <gz/msgs/stringmsg.pb.h>

#include <future>
#include <string>
#include <iostream>
#include <sstream>

#include "gz.hh"
#include "gz/transport/Node.hh"

#include <gz/utils/Environment.hh>

#include "test_utils.hh"

using namespace gz;

// Global constants.
static const std::string g_topic   = "/topic"; // NOLINT(*)
static const std::string g_service = "/echo"; // NOLINT(*)
static const std::string g_intType = "gz_msgs.Int32"; // NOLINT(*)
static const std::string g_reqData = "data: 10"; // NOLINT(*)

// Global variables.
static std::string     g_partition; // NOLINT(*)
static std::streambuf *g_stdOutFile;
static std::streambuf *g_stdErrFile;
static int             g_onewayData = 0;

// \brief Redirect stdout and stderr to streams.
void redirectIO(std::stringstream &_stdOutBuffer,
                std::stringstream &_stdErrBuffer)
{
  g_stdOutFile = std::cout.rdbuf(_stdOutBuffer.rdbuf());
  g_stdErrFile = std::cerr.rdbuf(_stdErrBuffer.rdbuf());
}

// \brief Clear all streams (including state flags).
void clearIOStreams(std::stringstream &_stdOutBuffer,
                    std::stringstream &_stdErrBuffer)
{
  _stdOutBuffer.str("");
  _stdOutBuffer.clear();
  _stdErrBuffer.str("");
  _stdErrBuffer.clear();
}

/// \brief Restore stdout and stderr redirections.
void restoreIO()
{
  std::cout.rdbuf(g_stdOutFile);
  std::cerr.rdbuf(g_stdErrFile);
}

/// \brief Provide a service.
bool srvEchoFail(const msgs::Int32 &_req, msgs::Int32 &_rep)
{
  _rep.set_data(_req.data());
  return false;
}

/// \brief Provide a service.
bool srvEchoOk(const msgs::Int32 &_req, msgs::Int32 &_rep)
{
  _rep.set_data(_req.data());
  return true;
}

/// \brief Provide a StringMsg echo service
bool srvEchoStringOk(const msgs::StringMsg &_req, msgs::StringMsg &_rep)
{
  _rep.set_data(_req.data());
  return true;
}

/// \brief Provide a one-way service.
void srvOnewayInt(const msgs::Int32 &_req)
{
  g_onewayData = _req.data();
}

/// \brief Provide a one-way service with a different request type.
void srvOnewayString(const msgs::StringMsg &)
{
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicInfo running the advertiser on a the same process.
TEST(gzTest, cmdTopicInfo)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  transport::Node node;

  // A null topic name should generate an error message.
  cmdTopicInfo(nullptr);
  EXPECT_EQ(stdErrBuffer.str(), "Invalid topic. Topic must not be empty.\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // A topic without advertisers should show an empty list of publishers.
  cmdTopicInfo(g_topic.c_str());
  EXPECT_TRUE(stdOutBuffer.str().find("No publishers on topic [/topic]\n") !=
    std::string::npos);
  EXPECT_TRUE(stdOutBuffer.str().find("No subscribers on topic [/topic]\n") !=
    std::string::npos);
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceInfo running the advertiser on a the same process.
TEST(gzTest, cmdServiceInfo)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  transport::Node node;

  // A null service name should generate an error message.
  cmdServiceInfo(nullptr);
  EXPECT_EQ(stdErrBuffer.str(),
    "Invalid service. Service must not be empty.\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // A service without advertisers should show no service providers.
  cmdServiceInfo(g_service.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "No service providers on service [/echo]\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicPub running the advertiser on a the same process.
TEST(gzTest, cmdTopicPub)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  transport::Node node;

  // A null topic name should generate an error message.
  cmdTopicPub(nullptr, g_intType.c_str(), g_reqData.c_str());
  EXPECT_EQ(stdErrBuffer.str(), "Topic name is null\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // A null msgType name should generate an error message.
  cmdTopicPub(g_topic.c_str(), nullptr, g_reqData.c_str());
  EXPECT_EQ(stdErrBuffer.str(), "Message type is null\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // Null data should generate an error message.
  cmdTopicPub(g_topic.c_str(), g_intType.c_str(), nullptr);
  EXPECT_EQ(stdErrBuffer.str(), "Message data is null\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq running the advertiser on a the same process.
TEST(gzTest, cmdServiceReq)
{
  std::stringstream  stdOutBuffer;
  std::stringstream  stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const std::string kUnknownType = "_unknown_type_";
  const int         kTimeout     = 10;

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_service, srvEchoFail));

  msgs::Int32 msg;
  msg.set_data(10);

  // A null service name should generate an error message.
  cmdServiceReq(nullptr, g_intType.c_str(), g_intType.c_str(),
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdErrBuffer.str(), "Service name is null\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // Null data should generate an error message.
  cmdServiceReq(g_service.c_str(), g_intType.c_str(),
    g_intType.c_str(), kTimeout, nullptr);
  EXPECT_EQ(stdErrBuffer.str(), "Request data is null\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // It's not possible to request a service using a request parameter that is
  // not part of Gazebo Messages.
  cmdServiceReq(g_service.c_str(), kUnknownType.c_str(),
    g_intType.c_str(), kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdErrBuffer.str(),
    "Unable to create request of type[_unknown_type_] with data[data: 10].\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // It's not possible to request a service using a response type that is not
  // part of Gazebo Messages.
  cmdServiceReq(g_service.c_str(), g_intType.c_str(),
    kUnknownType.c_str(), kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdErrBuffer.str(),
    "Unable to create response of type[_unknown_type_].\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // The service request is valid, received and containing a "false" result.
  cmdServiceReq(g_service.c_str(), g_intType.c_str(),
    g_intType.c_str(), kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "Service call failed\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // The service request is valid but will expire because there's no service
  // available.
  cmdServiceReq("_unknown_service_", g_intType.c_str(),
    g_intType.c_str(), kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdErrBuffer.str(), "Service call timed out\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq with type resolution
TEST(gzTest, cmdServiceReqInferTypes)
{
  std::stringstream  stdOutBuffer;
  std::stringstream  stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const std::string kUnknownType = "_unknown_type_";
  const int         kTimeout     = 10;
  const int         value        = 10;
  const std::string value_s      = std::to_string(value);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(g_service, srvEchoOk));

  msgs::Int32 msg;
  msg.set_data(value);

  // A null service request type should be automatically inferred. In
  // verbose mode the inferred type is reported on stderr.
  cmdServiceReq(g_service.c_str(), nullptr, g_intType.c_str(),
    kTimeout, g_reqData.c_str(), 1);
  EXPECT_EQ(stdOutBuffer.str(), "data: " + value_s + "\n\n");
  EXPECT_EQ(stdErrBuffer.str(), "Inferred types: request=gz.msgs.Int32\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // A null service response type should be automatically inferred
  cmdServiceReq(g_service.c_str(), g_intType.c_str(), nullptr,
    kTimeout, g_reqData.c_str(), 1);
  EXPECT_EQ(stdOutBuffer.str(), "data: " + value_s + "\n\n");
  EXPECT_EQ(stdErrBuffer.str(), "Inferred types: response=gz.msgs.Int32\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  cmdServiceReq(g_service.c_str(), nullptr, nullptr,
    kTimeout, g_reqData.c_str(), 1);
  EXPECT_EQ(stdOutBuffer.str(), "data: " + value_s + "\n\n");
  EXPECT_EQ(stdErrBuffer.str(),
    "Inferred types: request=gz.msgs.Int32, response=gz.msgs.Int32\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // Without verbose mode the inference is silent.
  cmdServiceReq(g_service.c_str(), nullptr, nullptr,
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "data: " + value_s + "\n\n");
  EXPECT_EQ(stdErrBuffer.str(), "");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq with type resolution and no service providers
TEST(gzTest, cmdServiceReqNoProviders)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const int kTimeout = 100;

  // The types cannot be resolved because nobody advertises the service.
  cmdServiceReq("/unadvertised", nullptr, nullptr,
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(),
    "No service providers on service [/unadvertised] after waiting 100 ms.\n"
    "Use --reqtype and --reptype to request without discovery.\n");

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq with type resolution and an invalid service
/// name.
TEST(gzTest, cmdServiceReqInvalidService)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const int kTimeout = 10;

  // The types cannot be resolved because the service name is not valid.
  cmdServiceReq("invalid service", nullptr, nullptr,
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(),
    "Service [invalid service] is not valid.\n");

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check that an explicit "gz.msgs.Empty" response type (what the
/// --oneway flag maps to) disambiguates between a two-way and a one-way
/// provider on the same service.
TEST(gzTest, cmdServiceReqOnewayDisambiguation)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const int kTimeout = 10;
  const std::string service = "/mixed_echo";
  g_onewayData = 0;

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise(service, srvEchoOk));
  EXPECT_TRUE(node2.Advertise(service, srvOnewayInt));

  // Without any explicit type the service is ambiguous. One of the
  // providers is one-way, so --oneway is suggested.
  cmdServiceReq(service.c_str(), nullptr, nullptr,
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(),
      "Ambiguous service types for service [/mixed_echo]:\n"
      "  request=gz.msgs.Int32, response=gz.msgs.Int32\n"
      "  request=gz.msgs.Int32, response=gz.msgs.Empty\n"
      "Use --reqtype and --reptype to specify explicitly.\n"
      "Use --oneway to select the one-way provider.\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // An explicit "gz.msgs.Empty" response type resolves the request type
  // (all the providers agree on it) and reaches the one-way provider.
  cmdServiceReq(service.c_str(), nullptr, "gz.msgs.Empty",
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(), "");
  EXPECT_EQ(10, g_onewayData);
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // An explicit two-way response type selects the two-way provider.
  cmdServiceReq(service.c_str(), nullptr, "gz.msgs.Int32",
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "data: 10\n\n");
  EXPECT_EQ(stdErrBuffer.str(), "");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // The underscore spelling of a type is normalized, so it selects the
  // one-way provider just like the canonical spelling does.
  g_onewayData = 0;
  cmdServiceReq(service.c_str(), nullptr, "gz_msgs.Empty",
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(), "");
  EXPECT_EQ(10, g_onewayData);

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check that a one-way request with an inferred type is reported,
/// rather than silently dropped, when no provider offers the requested
/// types, while explicit types remain best effort.
TEST(gzTest, cmdServiceReqOnewayNoCompatibleProvider)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const int kTimeout = 10;
  const std::string service = "/twoway_only";

  // The only provider is two-way.
  transport::Node node;
  EXPECT_TRUE(node.Advertise(service, srvEchoOk));

  // Asking for a one-way request cannot be served by it. The request type
  // is inferred, so the resolver reports the incompatibility.
  cmdServiceReq(service.c_str(), nullptr, "gz.msgs.Empty",
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(),
      "No provider on service [/twoway_only] offers "
      "response type [gz.msgs.Empty].\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  // With both types given explicitly discovery is not consulted: the
  // request is sent best effort and nothing is reported.
  cmdServiceReq(service.c_str(), "gz.msgs.Int32", "gz.msgs.Empty",
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(), "");

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check that --oneway is not suggested when the response type is
/// already explicit and the request type is the ambiguous one.
TEST(gzTest, cmdServiceReqOnewayAmbiguousRequest)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const int kTimeout = 10;
  const std::string service = "/oneway_ambiguous";

  transport::Node node1;
  transport::Node node2;

  // Two one-way providers that disagree on the request type.
  EXPECT_TRUE(node1.Advertise(service, srvOnewayInt));
  EXPECT_TRUE(node2.Advertise(service, srvOnewayString));

  cmdServiceReq(service.c_str(), nullptr, "gz.msgs.Empty",
    kTimeout, g_reqData.c_str());
  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(),
      "Ambiguous service types for service [/oneway_ambiguous]:\n"
      "  request=gz.msgs.Int32, response=gz.msgs.Empty\n"
      "  request=gz.msgs.StringMsg, response=gz.msgs.Empty\n"
      "Use --reqtype and --reptype to specify explicitly.\n");

  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdServiceReq with ambiguous types
TEST(gzTest, cmdServiceReqAmbiguousTypes)
{
  std::stringstream stdOutBuffer;
  std::stringstream stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const int kTimeout = 10;
  const std::string service = "/ambiguous_echo";

  transport::Node node1;
  transport::Node node2;
  transport::Node node3;

  // node3 repeats node1's types: the error should list each distinct
  // request/response pair only once.
  EXPECT_TRUE(node1.Advertise(service, srvEchoOk));
  EXPECT_TRUE(node2.Advertise(service, srvEchoStringOk));
  EXPECT_TRUE(node3.Advertise(service, srvEchoOk));

  cmdServiceReq(service.c_str(), nullptr, nullptr,
      kTimeout, g_reqData.c_str());

  EXPECT_EQ(stdOutBuffer.str(), "");
  EXPECT_EQ(stdErrBuffer.str(),
      "Ambiguous service types for service [/ambiguous_echo]:\n"
      "  request=gz.msgs.Int32, response=gz.msgs.Int32\n"
      "  request=gz.msgs.StringMsg, response=gz.msgs.StringMsg\n"
      "Use --reqtype and --reptype to specify explicitly.\n");
  restoreIO();
}

//////////////////////////////////////////////////
/// \brief Check cmdTopicEcho running the advertiser on a the same process.
TEST(gzTest, cmdTopicEcho)
{
  std::stringstream  stdOutBuffer;
  std::stringstream  stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  const std::string kInvalidTopic = "/";
  transport::Node node;

  // Requesting a null topic should trigger an error message.
  cmdTopicEcho(nullptr, 10.00, 0, MsgOutputFormat::kDefault);
  EXPECT_EQ(stdErrBuffer.str(), "Invalid topic. Topic must not be empty.\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  cmdTopicEcho(kInvalidTopic.c_str(), 5.00, 0, MsgOutputFormat::kDefault);
  EXPECT_EQ(stdErrBuffer.str(), "Topic [/] is not valid.\n");
  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

/////////////////////////////////////////////////
TEST(gzTest, cmdTopicEchoOutputFormats)
{
  std::stringstream  stdOutBuffer;
  std::stringstream  stdErrBuffer;
  redirectIO(stdOutBuffer, stdErrBuffer);

  transport::Node node;
  gz::msgs::Int32 msg;
  msg.set_data(5);

  clearIOStreams(stdOutBuffer, stdErrBuffer);

  auto getSubscriberOutput = [&](MsgOutputFormat _outputFormat)
  {
    cmdTopicEcho(g_topic.c_str(), 3.00, 1, _outputFormat);
    return stdOutBuffer.str();
  };

  auto defaultOutput = std::async(std::launch::async, getSubscriberOutput,
                                  MsgOutputFormat::kDefault);

  std::string str;
  ASSERT_TRUE(google::protobuf::TextFormat::PrintToString(msg, &str));
  cmdTopicPub(g_topic.c_str(), g_intType.c_str(), str.c_str());
  EXPECT_EQ("data: 5\n\n", defaultOutput.get());

  clearIOStreams(stdOutBuffer, stdErrBuffer);

  auto jsonOutput = std::async(std::launch::async, getSubscriberOutput,
                               MsgOutputFormat::kJSON);

  msg.set_data(10);
  ASSERT_TRUE(google::protobuf::TextFormat::PrintToString(msg, &str));
  cmdTopicPub(g_topic.c_str(), g_intType.c_str(), str.c_str());
  EXPECT_EQ("{\"data\":10}\n", jsonOutput.get());

  clearIOStreams(stdOutBuffer, stdErrBuffer);

  restoreIO();
}

/////////////////////////////////////////////////
/// Main
int main(int argc, char **argv)
{
  // Get a random partition name.
  g_partition = testing::getRandomNumber();

  // Set the partition name for this process.
  gz::utils::setenv("GZ_PARTITION", g_partition);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
