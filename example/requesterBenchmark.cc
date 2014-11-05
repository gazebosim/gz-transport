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

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <ignition/transport.hh>
#include "msg/benchmark.pb.h"

//////////////////////////////////////////////////
/// \brief Provide an "echo" service.
void srvEcho(const std::string &_topic,
  const example::mymsgs::BenchmarkMsg &_req,
  example::mymsgs::BenchmarkMsg &_rep, bool &_result)
{
  // Set the response's content.
  _rep.add_data(_req.data(0));

  // The response succeed.
  _result = true;
}

bool sendRequest(ignition::transport::Node &_node,
  const std::string _topic, int _numBlocks, std::chrono::microseconds &_elapsed)
{
   // Prepare the input parameters.
  example::mymsgs::BenchmarkMsg req;

  for (auto i = 0; i < _numBlocks; ++i)
    req.add_data(1);

  example::mymsgs::BenchmarkMsg rep;
  bool result;
  unsigned int timeout = 10000;

  // 4 byte message size.
  auto sent = std::chrono::steady_clock::now();

  // Request the "/echo" service.
  auto executed = _node.Request(_topic, req, timeout, rep, result);
  auto received = std::chrono::steady_clock::now();
  // Elapsed time since the request was sent.
  _elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
    received - sent);

  return executed && result;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Configure the different message sizes. The value represents the message
  // size in blocks of 4 bytes.
  std::map<int, std::string> sizes;

  // 4B.
  sizes[exp2(2) / 4] = "4B";
  // 32B.
  sizes[exp2(5) / 4] = "32B";
  // 128B.
  sizes[exp2(7) / 4] = "128B";
  // 1KB.
  sizes[exp2(10) / 4] = "1 KB";
  // 100KB.
  sizes[100 * exp2(10) / 4] = "100 KB";
  // 1 MB.
  sizes[exp2(20) / 4] = "1 MB";
  // 10 MB.
  sizes[10 * exp2(20) / 4] = "10 MB";
  // 100 MB.
  sizes[100 * exp2(20) / 4] = "100 MB";

  // Different configurations: same process, same host, same LAN.
  std::map<std::string, std::string> topics;

  // Communication inside the same process.
  topics["/process/echo"] = "inproc";
  // Communication between different processes in the same machine.
  topics["/host/echo"] = "host";
  // Communication between different machines in the same LAN.
  //topics["/LAN/echo"] = "LAN";

    // Create a transport node.
  ignition::transport::Node node;

  // Advertise a service call.
  node.Advertise("/process/echo", srvEcho);

  std::chrono::microseconds elapsed;

  for (auto &topic : topics)
  {
    std::cout << "Configuration: " << topic.second << std::endl;

    // Send a first a message to trigger the discovery.
    sendRequest(node, topic.first, 1, elapsed);

    for (auto &s : sizes)
    {
      // Results.
      boost::accumulators::accumulator_set<int, boost::accumulators::features<
        boost::accumulators::tag::count, boost::accumulators::tag::mean,
        boost::accumulators::tag::median, boost::accumulators::tag::variance>>
        acc;

      std::cout << "\tSize: " << s.second << std::endl;
      for (auto counter = 0; counter < 5; ++counter)
      {
        if (sendRequest(node, topic.first, s.first, elapsed))
        {
          std::cout << "\t\tElapsed: " << elapsed.count() << " us. / " <<
                       elapsed.count() / 1000.0 << "ms." << std::endl;
          acc(elapsed.count());
        }
        else
        {
          std::cout << "\t\tElapsed: Lost!" << std::endl;
        }
      }

      std::cout << "\n\t\tSummary:" << std::endl;
      std::cout << "\t\t\tMean: " << boost::accumulators::mean(acc) << " us. / "
                << boost::accumulators::mean(acc) / 1000.0 << " ms."
                << std::endl;
      std::cout << "\t\t\tMedian: " << boost::accumulators::median(acc)
                << " us. / " << boost::accumulators::median(acc) / 1000.0
                << " ms." << std::endl;
      std::cout << "\t\t\tStd dev: " << sqrt(boost::accumulators::variance(acc))
                << " us. / "
                << sqrt(boost::accumulators::variance(acc)) / 1000.0
                << " ms." << std::endl;
    }
  }
}
