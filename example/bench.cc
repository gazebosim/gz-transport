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

#include <gflags/gflags.h>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <ignition/msgs.hh>
#include <ignition/transport.hh>

DEFINE_bool(t, false, "Throughput testing");
DEFINE_bool(l, false, "Latency testing");
DEFINE_bool(r, false, "Relay node");
DEFINE_bool(p, false, "Publishing node");

std::condition_variable gCondition;
std::mutex gMutex;

class ReplyTester
{
  public: ReplyTester()
  {
    this->throughputPub = this->node.Advertise<ignition::msgs::Bytes>(
        "/benchmark/throughput/reply");
    if (!this->throughputPub)
    {
      std::cerr << "Error advertising topic /benchmark/throughput/reply"
                << std::endl;
      return;
    }

    this->latencyPub = this->node.Advertise<ignition::msgs::Bytes>(
        "/benchmark/latency/reply");
    if (!this->latencyPub)
    {
      std::cerr << "Error advertising topic /benchmark/latency/reply"
                << std::endl;
      return;
    }

    // Subscribe to the subTopic by registering a callback.
    if (!node.Subscribe("/benchmark/throughput/request",
          &ReplyTester::ThroughputCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/throughput/request"
                << std::endl;
      return;
    }

    // Subscribe to the subTopic by registering a callback.
    if (!node.Subscribe("/benchmark/latency/request",
          &ReplyTester::LatencyCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/throughput/request"
                << std::endl;
      return;
    }

    // Kick discovery. Need to fix this..
    std::vector<std::string> topics;
    this->node.TopicList(topics);
  }

  /// \brief Function called each time a topic update is received.
  private: void ThroughputCb(const ignition::msgs::Bytes &_msg)
  {
    this->throughputPub.Publish(_msg);
  }

  /// \brief Function called each time a topic update is received.
  private: void LatencyCb(const ignition::msgs::Bytes &_msg)
  {
    this->latencyPub.Publish(_msg);
  }


  private: ignition::transport::Node node;
  private: ignition::transport::Node::Publisher throughputPub;
  private: ignition::transport::Node::Publisher latencyPub;
};

class PubTester
{
  /// \brief Constructor
  public: PubTester()
  {
    this->throughputPub = this->node.Advertise<ignition::msgs::Bytes>(
        "/benchmark/throughput/request");
    if (!this->throughputPub)
    {
      std::cerr << "Error advertising topic /benchmark/throughput/request"
                << std::endl;
      return;
    }

    this->latencyPub = this->node.Advertise<ignition::msgs::Bytes>(
        "/benchmark/latency/request");
    if (!this->latencyPub)
    {
      std::cerr << "Error advertising topic /benchmark/latency/request"
                << std::endl;
      return;
    }

    // Subscribe to the subTopic by registering a callback.
    if (!node.Subscribe("/benchmark/throughput/reply",
                        &PubTester::ThroughputCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/throughput/reply"
                << std::endl;
      return;
    }

    // Subscribe to the subTopic by registering a callback.
    if (!node.Subscribe("/benchmark/latency/reply",
                        &PubTester::LatencyCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/latency/reply"
                << std::endl;
      return;
    }

    // Kick discovery. Need to fix this..
    std::vector<std::string> topics;
    this->node.TopicList(topics);
  }

  /// \brief Measure throughput.
  public: void Throughput()
  {
    std::cout << "Msg Size\tMB/s\tKmsg/s\n";

    for (auto msgSize : this->msgSizes)
    {
      this->totalBytes = 0;
      this->msgCount = 0;

      this->PrepMsg(msgSize);

      auto timeStart = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < this->sentMsgs; ++i)
      {
        this->throughputPub.Publish(this->msg);
      }

      std::unique_lock<std::mutex> lk(this->mutex);
      if (this->msgCount < this->sentMsgs)
        this->condition.wait(lk);
      auto timeEnd = std::chrono::high_resolution_clock::now();

      uint64_t duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            timeEnd - timeStart).count();

      double seconds = (duration * 1e-6);
      std::cout << this->dataSize << "\t\t"
        << (this->totalBytes * 1e-6) / seconds << "\t"
        << (this->msgCount * 1e-3) / seconds << "\t" <<  std::endl;
    }
  }

  /// \brief Measure latency.
  public: void Latency()
  {
    std::cout << "Msg Size\tLatency (us)\n";

    for (auto msgSize : this->msgSizes)
    {
      this->PrepMsg(msgSize);

      uint64_t sum = 0;

      for (int i = 0; i < this->sentMsgs; ++i)
      {
        std::unique_lock<std::mutex> lk(this->mutex);

        auto timeStart = std::chrono::high_resolution_clock::now();

        this->latencyPub.Publish(this->msg);
        this->condition.wait(lk);
        auto timeEnd = std::chrono::high_resolution_clock::now();

        uint64_t duration =
          std::chrono::duration_cast<std::chrono::microseconds>(
              timeEnd - timeStart).count();

        sum += duration;
      }

      std::cout << this->dataSize << "\t\t"
                << (sum / (double)this->sentMsgs) * 0.5 << std::endl;
    }
  }

  /// \brief Function called each time a topic update is received.
  private: void ThroughputCb(const ignition::msgs::Bytes &_msg)
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    this->totalBytes += this->dataSize;
    this->msgCount++;
    if (this->msgCount >= this->sentMsgs)
      condition.notify_all();
  }

  /// \brief Function called each time a topic update is received.
  private: void LatencyCb(const ignition::msgs::Bytes &_msg)
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    this->condition.notify_all();
  }

  private: void PrepMsg(int _size)
  {
    // Prepare the message.
    char *byteData = new char[_size];
    for (auto i = 0; i < _size; ++i)
      byteData[i] = '0';
    msg.set_data(byteData);

    std::string data;
    this->msg.SerializeToString(&data);
    this->dataSize = data.size();
  }

  private: std::vector<int> msgSizes =
    {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      128000, 256000, 512000, 1000000, 2000000, 4000000
    };

  private: std::condition_variable condition;
  private: std::mutex mutex;

  private: ignition::msgs::Bytes msg;
  private: uint64_t dataSize = 0;
  private: uint64_t totalBytes = 0;
  private: uint64_t msgCount = 0;
  private: uint64_t sentMsgs = 1000;

  private: ignition::transport::Node node;
  private: ignition::transport::Node::Publisher throughputPub;
  private: ignition::transport::Node::Publisher latencyPub;
};

//////////////////////////////////////////////////
void signal_handler(int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
    gCondition.notify_all();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Install a signal handler for SIGINT and SIGTERM.
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::string usage("Benchmark testing program.");
  usage += " Usage:\n ./benchmark -t";

  gflags::SetUsageMessage(usage);

  gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);

  std::vector<gflags::CommandLineFlagInfo> flags;
  gflags::GetAllFlags(&flags);

  // Run the responder
  if (FLAGS_r)
  {
    ReplyTester replyTester;
    std::unique_lock<std::mutex> lk(gMutex);
    gCondition.wait(lk);
  }
  // Run the publisher
  else if (FLAGS_p)
  {
    PubTester pubTester;
    if (FLAGS_t)
      pubTester.Throughput();
    else
      pubTester.Latency();
  }
  // Single process with both publisher and responder
  else
  {
    ReplyTester replyTester;
    PubTester pubTester;

    if (FLAGS_t)
      pubTester.Throughput();
    else
      pubTester.Latency();
  }
  return 0;
}
