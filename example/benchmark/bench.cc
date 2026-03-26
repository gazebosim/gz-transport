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

//////////////////////////////////////////////////
/// Usage: ./bench <options>
///
/// Options:
///
/// -h Help
/// -l Latency test
/// -t Throughput test
/// -p Publish/caller node
/// -r Reply/server node
/// -c  Two-way service call mode (req+rep)
/// --oneway  One-way service call mode (req only, throughput only)
/// --noinput No-input service call mode (rep only, no req)
/// -s Extended SHM sizes (bracket 128KB threshold)
/// -w Warmup iterations (default 10)
///
/// Choose one of [-l, -t], optionally one of [-c, --oneway, --noinput] for
/// service calls, and one (or none for in-process testing) of [-p, -r].
/// Note: --oneway does not support -l (no reply to measure latency).
///
/// See `plot_benchmark.py` to plot output.
//////////////////////////////////////////////////

#ifdef __linux__
#include <sys/utsname.h>
#include <unistd.h>
#endif

#include <gflags/gflags.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <gz/msgs.hh>
#include <gz/transport.hh>

DEFINE_bool(h, false, "Show help");
DEFINE_bool(t, false, "Throughput testing");
DEFINE_bool(l, false, "Latency testing");
DEFINE_bool(r, false, "Relay node");
DEFINE_bool(p, false, "Publishing node");
DEFINE_bool(s, false, "Extended SHM message sizes (bracket 128KB threshold)");
DEFINE_uint64(f, 0, "Flood the network with extra publishers and subscribers");
DEFINE_uint64(i, 1000, "Number of iterations");
DEFINE_uint64(w, 10, "Number of warmup iterations per size");
DEFINE_string(o, "", "Output filename");
DEFINE_bool(c, false, "Service call benchmark mode (two-way, req+rep)");
DEFINE_bool(oneway, false, "One-way service call benchmark mode (req only, no reply)");
DEFINE_bool(noinput, false, "No-input service call benchmark mode (rep only, no req)");

std::condition_variable gCondition;
std::mutex gMutex;
bool gStop = false;

/// \brief A class that subscribes to all of the `/benchmark/flood/*`
/// topics. FloodSub and FloodPub can be enabled with the `-f <num>` command
/// line argument. Flooding adds <num> extra publishers and subscribers. The
/// purpose is to "flood" the network with extra messages while performing
/// benchmark analysis.
class FloodSub
{
  /// \brief Create the subscribers.
  /// \param[in] _count The number of subscribers to create.
  public: explicit FloodSub(uint64_t _count)
  {
    // Create flood subscribers
    for (uint64_t i = 0; i < _count; ++i)
    {
      std::ostringstream stream;
      stream << "/benchmark/flood/"  << i;
      if (!this->node.Subscribe(stream.str(), &FloodSub::OnMsg, this))
        std::cerr << "Failed to subscribe to " << stream.str() << std::endl;
    }
  }

  /// \brief Dummy callback.
  /// \param[in] _msg The message.
  public: void OnMsg(const gz::msgs::Bytes & /*_msg*/)
  {
  }

  /// \brief Communication node.
  private: gz::transport::Node node;
};

/// \brief A class that publishes on a number of `/benchmark/flood/*`
/// topics. FloodSub and FloodPub can be enabled with the `-f <num>` command
/// line argument. Flooding adds <num> extra publishers and subscribers. The
/// purpose is to "flood" the network with extra messages while performing
/// benchmark analysis.
class FloodPub
{
  /// \brief Create a number of publishers.
  /// \param[in] _count Number of publishers to create.
  public: explicit FloodPub(uint64_t _count)
  {
    // Create flood publishers
    for (uint64_t i = 0; i < _count; ++i)
    {
      std::ostringstream stream;
      stream << "/benchmark/flood/"  << i;
      auto pub = this->node.Advertise<gz::msgs::Bytes>(stream.str());
      if (!pub)
        std::cerr << "Failed to advertise " << stream.str() << std::endl;
      else
        this->floodPubs.push_back(std::move(pub));
    }
    if (!this->floodPubs.empty())
      this->runThread = std::thread(&FloodPub::RunLoop, this);
  }

  /// \brief Destructor.
  public: ~FloodPub()
  {
    this->Stop();
    if (this->runThread.joinable())
      this->runThread.join();
  }

  /// \brief Stop the publishers.
  public: void Stop()
  {
    this->running = false;
  }

  /// \brief Run the publishers.
  private: void RunLoop()
  {
    gz::msgs::Bytes msg;
    int size = 1000;
    std::string byteData(size, '0');
    msg.set_data(byteData);

    this->running = true;
    while (this->running)
    {
      for (gz::transport::Node::Publisher &pub : this->floodPubs)
      {
        pub.Publish(msg);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  /// \brief Communication node.
  private: gz::transport::Node node;

  /// \brief Run thread.
  private: std::thread runThread;

  /// \brief True when running.
  private: bool running{false};

  /// \brief The publishers.
  private: std::vector<gz::transport::Node::Publisher> floodPubs;
};

/// \brief The ReplyTester subscribes to the benchmark topics, and relays
/// incoming messages on a corresponding "reply" topic.
///
/// A publisher should send messages on either:
///
///   1. /benchmark/latency/request For latency testing
///   2. /benchmark/throughput/request For throughput testing.
///
/// The incoming and outgoing message types are gz::msgs::Bytes.
class ReplyTester
{
  /// Constructor that creates the publishers and subscribers.
  public: ReplyTester()
  {
    // Advertise on the throughput reply topic
    this->throughputPub = this->node.Advertise<gz::msgs::Bytes>(
        "/benchmark/throughput/reply");
    if (!this->throughputPub)
    {
      std::cerr << "Error advertising topic /benchmark/throughput/reply"
                << std::endl;
      return;
    }

    // Advertise on the latency reply topic
    this->latencyPub = this->node.Advertise<gz::msgs::Bytes>(
        "/benchmark/latency/reply");
    if (!this->latencyPub)
    {
      std::cerr << "Error advertising topic /benchmark/latency/reply"
                << std::endl;
      return;
    }

    // Subscribe to the throughput request topic.
    if (!node.Subscribe("/benchmark/throughput/request",
          &ReplyTester::ThroughputCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/throughput/request"
                << std::endl;
      return;
    }

    // Subscribe to the latency request topic.
    if (!node.Subscribe("/benchmark/latency/request",
          &ReplyTester::LatencyCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/latency/request"
                << std::endl;
      return;
    }

    // Kick discovery.
    // \todo: Improve discovery so that this is not required.
    std::vector<std::string> topics;
    this->node.TopicList(topics);
  }

  /// \brief Function called each time a throughput message is received.
  /// Just relay - Zenoh with SHM may reorder under high throughput.
  /// \param[in] _msg Incoming message of variable size.
  private: void ThroughputCb(const gz::msgs::Bytes &_msg)
  {
    this->throughputPub.Publish(_msg);
  }

  /// \brief Function called each time a latency message is received.
  /// \param[in] _msg Incoming message of variable size.
  private: void LatencyCb(const gz::msgs::Bytes &_msg)
  {
    this->latencyPub.Publish(_msg);
  }

  /// \brief The transport node
  private: gz::transport::Node node;

  /// \brief The throughput publisher
  private: gz::transport::Node::Publisher throughputPub;

  /// \brief The latency publisher
  private: gz::transport::Node::Publisher latencyPub;
};

/// \brief Advertises a two-way echo service for service call benchmarking.
/// Run with -r -c to start the server side.
class SrvResponder
{
  public: SrvResponder()
  {
    if (!this->node.Advertise("/benchmark/service",
          &SrvResponder::OnRequest, this))
    {
      std::cerr << "Error advertising service /benchmark/service\n";
    }
  }

  private: bool OnRequest(const gz::msgs::Bytes &_req, gz::msgs::Bytes &_rep)
  {
    _rep = _req;
    return true;
  }

  private: gz::transport::Node node;
};

/// \brief Advertises a one-way service for benchmarking fire-and-forget calls.
/// The handler intentionally does nothing so server overhead is minimal.
/// Run with -r --oneway to start the server side.
class OneWayResponder
{
  public: OneWayResponder()
  {
    if (!this->node.Advertise("/benchmark/oneway",
          &OneWayResponder::OnRequest, this))
    {
      std::cerr << "Error advertising service /benchmark/oneway\n";
    }
  }

  private: void OnRequest(const gz::msgs::Bytes & /*_req*/) {}

  private: gz::transport::Node node;
};

/// \brief Advertises a no-input (reply-only) service for benchmarking.
///
/// Two services are provided:
///   /benchmark/noinput/configure  - Two-way: client sends Int32 (reply size
///                                   in bytes), server stores it.
///   /benchmark/noinput            - No-input: replies with Bytes of the
///                                   configured size.
///
/// Run with -r --noinput to start the server side.
class NoInputResponder
{
  public: NoInputResponder()
  {
    if (!this->node.Advertise("/benchmark/noinput/configure",
          &NoInputResponder::OnConfigure, this))
    {
      std::cerr << "Error advertising service /benchmark/noinput/configure\n";
    }
    if (!this->node.Advertise("/benchmark/noinput",
          &NoInputResponder::OnRequest, this))
    {
      std::cerr << "Error advertising service /benchmark/noinput\n";
    }
  }

  /// \brief Set the reply size for subsequent /benchmark/noinput calls.
  private: bool OnConfigure(const gz::msgs::Int32 &_req,
                             gz::msgs::Boolean &_rep)
  {
    this->replySize = _req.data();
    _rep.set_data(true);
    return true;
  }

  /// \brief Reply with Bytes of the configured size.
  private: bool OnRequest(gz::msgs::Bytes &_rep)
  {
    _rep.set_data(std::string(this->replySize, '0'));
    return true;
  }

  /// \brief Desired reply data size in bytes (set via /configure).
  private: int replySize = 1000;

  private: gz::transport::Node node;
};

/// \brief The PubTester is used to collect data on latency or throughput.
/// Latency is the measure of time from message publication to message
/// reception. Latency is calculated by dividing the complete roundtrip
/// time of a message in half. This avoids time synchronization issues.
///
/// Throughput is measured by sending N messages, and measuring the time
/// required to send those messages. Again, half of the complete roundtrip
/// time is used to avoid time synchronization issues.
///
/// The latency topics are:
///
///   1. /benchmark/latency/request Outbound data, sent by this class.
///   2. /benchmark/latency/reply Inbound data, sent by ReplyTester.
///
/// The throughput topics are:
///
///   1. /benchmark/throughput/request Outbound data, sent by this class.
///   2. /benchmark/throughput/reply Inbound data, sent by ReplyTester.
class PubTester
{
  /// \brief Default constructor.
  public: PubTester() = default;

  /// \brief Set the output filename. Use empty string to output to the
  /// console.
  /// \param[in] _filename Output filename
  public: void SetOutputFilename(const std::string &_filename)
  {
    this->filename = _filename;
  }

  /// \brief Set the number of iterations.
  /// \param[in] _iters Number of iterations.
  public: void SetIterations(const uint64_t _iters)
  {
    this->sentMsgs = _iters;
  }

  /// \brief Set the number of warmup iterations.
  /// \param[in] _warmup Number of warmup iterations per size.
  public: void SetWarmup(const uint64_t _warmup)
  {
    this->warmupIters = _warmup;
  }

  /// \brief Start a background spinner that overwrites a single stderr line
  /// every 200ms, showing size index, iteration progress, elapsed time and a
  /// spinning character.  Acts as a deadman switch: if the line stops
  /// changing the process is stuck; if it ticks it is just working hard.
  /// \param[in] _sizeIdx  Current size index (1-based).
  /// \param[in] _sizeTotal Total number of sizes.
  /// \param[in] _iterTotal Total iterations for this size.
  /// \param[in] _label Short label string, e.g. "latency" or "throughput".
  private: void StartProgress(std::size_t _sizeIdx, std::size_t _sizeTotal,
                               uint64_t _iterTotal,
                               const std::string &_label)
  {
    this->progressSizeIdx = _sizeIdx;
    this->progressSizeTotal = _sizeTotal;
    this->progressTotal = _iterTotal;
    this->progressLabel = _label;
    this->progressIter = 0;
    this->progressStart = std::chrono::steady_clock::now();
    this->progressRunning = true;
    this->progressThread = std::thread(
      [this, _sizeIdx, _sizeTotal, _iterTotal, _label]()
      {
        static const char spin[] = "|/-\\";
        int s = 0;
        while (this->progressRunning.load())
        {
          auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - this->progressStart).count();
          uint64_t it = this->progressIter.load();
          double pct = (_iterTotal > 0)
            ? 100.0 * it / _iterTotal : 0.0;
          std::cerr
            << "\r  [" << _sizeIdx << "/" << _sizeTotal << "] "
            << _label << " " << this->dataSize << "B  "
            << it << "/" << _iterTotal
            << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
            << std::setprecision(1) << ms / 1000.0 << "s  "
            << spin[s++ & 3]
            << "     " << std::flush;
          std::unique_lock<std::mutex> lk(this->progressWakeMutex);
          this->progressWakeCv.wait_for(lk, std::chrono::milliseconds(200),
            [this] { return !this->progressRunning.load(); });
        }
      });
  }

  /// \brief Stop the background spinner and print the final state.
  private: void StopProgress()
  {
    this->progressRunning = false;
    this->progressWakeCv.notify_all();
    if (this->progressThread.joinable())
      this->progressThread.join();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - this->progressStart).count();
    uint64_t it = this->progressIter.load();
    double pct = (this->progressTotal > 0)
      ? 100.0 * it / this->progressTotal : 0.0;
    std::cerr
      << "\r  [" << this->progressSizeIdx << "/" << this->progressSizeTotal
      << "] " << this->progressLabel << " " << this->dataSize << "B  "
      << it << "/" << this->progressTotal
      << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
      << std::setprecision(1) << ms / 1000.0 << "s  done"
      << "     \n" << std::flush;
  }

  /// \brief Use extended SHM sizes that bracket the 128KB threshold.
  public: void SetShmSizes()
  {
    this->msgSizes = {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      96000, 112000, 120000, 128000, 136000, 144000, 160000, 192000,
      256000, 512000, 1000000, 2000000, 4000000
    };
  }

  /// \brief Create the publishers and subscribers.
  public: void Init()
  {
    // Throughput publisher
    this->throughputPub = this->node.Advertise<gz::msgs::Bytes>(
        "/benchmark/throughput/request");
    if (!this->throughputPub)
    {
      std::cerr << "Error advertising topic /benchmark/throughput/request"
                << std::endl;
      return;
    }

    // Latency publisher
    this->latencyPub = this->node.Advertise<gz::msgs::Bytes>(
        "/benchmark/latency/request");
    if (!this->latencyPub)
    {
      std::cerr << "Error advertising topic /benchmark/latency/request"
                << std::endl;
      return;
    }

    // Subscribe to the throughput reply topic.
    if (!node.Subscribe("/benchmark/throughput/reply",
                        &PubTester::ThroughputCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/throughput/reply"
                << std::endl;
      return;
    }

    // Subscribe to the latency reply topic.
    if (!node.Subscribe("/benchmark/latency/reply",
                        &PubTester::LatencyCb, this))
    {
      std::cerr << "Error subscribing to topic /benchmark/latency/reply"
                << std::endl;
      return;
    }

    // Kick discovery.
    // \todo: Improve discovery so that this is not required.
    std::vector<std::string> topics;
    this->node.TopicList(topics);
  }

  /// \brief Used to stop the test.
  public: void Stop()
  {
    std::unique_lock<std::mutex> lk(this->mutex);
    this->stop = true;
    this->condition.notify_all();
    this->progressWakeCv.notify_all();
  }

  /// \brief Output header information
  /// \param[in] _stream Stream pointer
  private: void OutputHeader(std::ostream *_stream)
  {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);

    (*_stream) << "# " << std::put_time(&tm, "%FT%T%Z") << std::endl;
    (*_stream) << "# Gazebo Transport Version "
               << GZ_TRANSPORT_VERSION_FULL << std::endl;

#ifdef __linux__
    struct utsname unameData;
    uname(&unameData);
    (*_stream) << "# " << unameData.sysname << " " << unameData.release
               << " " << unameData.version << " " << unameData.machine
               << std::endl;
#endif

    // Log transport configuration for reproducibility.
    // Always print all fields (with defaults) so .dat files are self-describing.
    const char *impl = std::getenv("GZ_TRANSPORT_IMPLEMENTATION");
    const char *shm = std::getenv("GZ_TRANSPORT_ZENOH_SHM");
    const char *shmPool = std::getenv("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE");
    const char *shmThresh = std::getenv("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD");
    const char *cc = std::getenv("GZ_TRANSPORT_ZENOH_CONGESTION_CONTROL");
    const char *txq = std::getenv("GZ_TRANSPORT_ZENOH_TX_QUEUE_SIZE");

    (*_stream) << "# Backend: " << (impl ? impl : "zeromq (default)")
               << std::endl;
    (*_stream) << "# SHM: " << (shm ? shm : "enabled (default)") << std::endl;
    (*_stream) << "# SHM pool: "
               << (shmPool ? shmPool : "10485760 (default)") << std::endl;
    (*_stream) << "# SHM threshold: "
               << (shmThresh ? shmThresh : "131072 (default)") << std::endl;
    (*_stream) << "# Congestion: " << (cc ? cc : "drop (default)") << std::endl;
    (*_stream) << "# TX queue: " << (txq ? txq : "2 (default)") << std::endl;
    (*_stream) << "# Iterations: " << this->sentMsgs
               << "  Warmup: " << this->warmupIters << std::endl;
  }

  /// \brief Measure throughput. The output contains columns:
  ///    1. Test number
  ///    2. Message size in bytes
  ///    3. Throughput in megabytes per second
  ///    4. Throughput in thousands of messages per second
  ///    5. Message loss percentage
  public: void Throughput()
  {
    // Wait for subscriber
    while (!this->throughputPub.HasConnections() && !this->stop)
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Short circuit in case this test was stopped while waiting for
    // a subscriber
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;

    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);

    // Column headers.
    (*stream) << "# Test\tSize(B)\t\tMB/s\t\tKmsg/s\t\tLoss%\n";

    int testNum = 1;
    // Iterate over each of the message sizes
    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      // Create the message of the given size
      this->PrepMsg(msgSize);

      // Warmup: send a few messages to prime the pipeline (SHM pool,
      // Zenoh batching, etc.) without measuring.
      if (this->warmupIters > 0)
      {
        this->totalBytes = 0;
        this->msgCount = 0;
        this->outOfOrder = 0;
        for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        {
          this->msg.mutable_header()->mutable_stamp()->set_sec(i);
          this->throughputPub.Publish(this->msg);
        }
        // Wait briefly for warmup replies to drain.
        // Scale timeout to iteration count: 20ms per msg, min 2s, max 30s.
        auto warmupTimeout = std::chrono::milliseconds(
          std::min(30000UL,
            std::max(2000UL, this->warmupIters * 20UL)));
        {
          std::unique_lock<std::mutex> lk(this->mutex);
          this->condition.wait_for(lk, warmupTimeout, [this] {
              return gStop || this->msgCount >= this->warmupIters;});
        }
      }

      // Reset counters for the actual measurement
      this->totalBytes = 0;
      this->msgCount = 0;
      this->outOfOrder = 0;

      // Start the clock and spinner
      auto timeStart = std::chrono::high_resolution_clock::now();
      this->StartProgress(testNum, this->msgSizes.size(),
                          this->sentMsgs, "throughput");

      // Send all the messages as fast as possible
      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        this->msg.mutable_header()->mutable_stamp()->set_sec(i);
        this->throughputPub.Publish(this->msg);
      }

      // Wait for all the reply messages with a timeout to handle loss.
      // Scale timeout to iteration count: 20ms per msg, min 2s, max 30s.
      auto measureTimeout = std::chrono::milliseconds(
        std::min(30000UL,
          std::max(2000UL, this->sentMsgs * 20UL)));
      {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->condition.wait_for(lk, measureTimeout, [this] {
            return gStop || this->msgCount >= this->sentMsgs;});
      }

      // End timing when we either received all messages or timed out
      this->timeEnd = std::chrono::high_resolution_clock::now();
      this->StopProgress();

      // Compute the number of microseconds
      uint64_t duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            this->timeEnd - timeStart).count();

      // Convert to seconds
      double seconds = (duration * 1e-6);

      // Compute loss percentage
      double lossPct = 0.0;
      if (this->sentMsgs > 0)
      {
        uint64_t received = this->msgCount;
        lossPct = 100.0 *
            (1.0 - static_cast<double>(received) /
             static_cast<double>(this->sentMsgs));
        if (lossPct < 0.0) lossPct = 0.0;
      }

      // Output the data
      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize << "\t\t"
                << (this->totalBytes * 1e-6) / seconds << "\t"
                << (this->msgCount * 1e-3) / seconds << "\t"
                << lossPct << std::endl;
    }
  }

  /// \brief Measure latency. The output contains columns:
  ///    1. Test number
  ///    2. Message size in bytes
  ///    3. Average latency in microseconds
  ///    4. Min latency in microseconds
  ///    5. Max latency in microseconds
  public: void Latency()
  {
    // Wait for subscriber
    while (!this->latencyPub.HasConnections() && !this->stop)
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Short circuit in case this test was stopped while waiting for
    // a subscriber
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;

    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);

    // Column headers.
    (*stream) << "# Test\tSize(B)\tAvg_(us)\tMin_(us)\tMax_(us)\n";

    uint64_t maxLatency = 0;
    uint64_t minLatency = std::numeric_limits<uint64_t>::max();
    int testNum = 1;
    // Iterate over each of the message sizes
    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      // Create the message of the given size
      this->PrepMsg(msgSize);

      // Warmup: prime the pipeline (SHM pool, Zenoh session, kernel buffers)
      // before recording any measurements.
      if (this->warmupIters > 0)
      {
        for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        {
          std::unique_lock<std::mutex> lk(this->mutex);
          auto warmStart = std::chrono::high_resolution_clock::now();
          this->timeEnd = warmStart;
          this->latencyPub.Publish(this->msg);
          this->condition.wait_for(lk, std::chrono::seconds(5),
              [this, &warmStart] {
              return gStop || this->timeEnd > warmStart;});
        }
      }

      uint64_t sum = 0;
      uint64_t receivedCount = 0;

      // Start spinner — each iteration of the inner loop increments
      // progressIter so the spinner shows live forward motion.
      this->StartProgress(testNum, this->msgSizes.size(),
                          this->sentMsgs, "latency");

      // Send each message.
      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        // Lock so that we wait on a condition variable.
        std::unique_lock<std::mutex> lk(this->mutex);

        // Start the clock
        auto timeStart = std::chrono::high_resolution_clock::now();
        this->timeEnd = timeStart;

        // Send the message.
        this->latencyPub.Publish(this->msg);

        // Wait for the response with a timeout.
        this->condition.wait_for(lk, std::chrono::seconds(10),
            [this, &timeStart] {
            return gStop || this->timeEnd > timeStart;});

        // Only record samples where a reply was actually received.
        // Timed-out iterations (timeEnd == timeStart) are skipped so
        // they don't dilute the average toward zero.
        if (this->timeEnd > timeStart)
        {
          uint64_t duration =
            std::chrono::duration_cast<std::chrono::microseconds>(
                this->timeEnd - timeStart).count();

          if (duration > maxLatency)
            maxLatency = duration;
          if (duration < minLatency)
            minLatency = duration;

          sum += duration;
          receivedCount++;
        }

        // Advance spinner counter
        this->progressIter = i + 1;
      }

      this->StopProgress();

      // Output data. Divide by receivedCount (not sentMsgs) so timed-out
      // iterations don't skew the average toward zero.
      double avgUs = (receivedCount > 0)
        ? (sum / static_cast<double>(receivedCount)) * 0.5 : 0.0;
      double minUs = (receivedCount > 0) ? minLatency * 0.5 : 0.0;
      double maxUs = (receivedCount > 0) ? maxLatency * 0.5 : 0.0;
      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize << "\t"
                << avgUs << "\t" << minUs << "\t" << maxUs << std::endl;
    }
  }

  /// \brief Callback that handles throughput replies
  /// \param[in] _msg The reply message
  private: void ThroughputCb(const gz::msgs::Bytes &_msg)
  {
    // Lock
    std::unique_lock<std::mutex> lk(this->mutex);

    // Add to the total bytes received.
    this->totalBytes += this->dataSize;

    // Add to the total messages received.
    this->msgCount++;
    this->progressIter = this->msgCount;

    // Track out-of-order messages (informational only - Zenoh with SHM may
    // reorder under high throughput).
    if (_msg.header().stamp().sec() !=
        static_cast<int64_t>(this->msgCount - 1))
    {
      this->outOfOrder++;
    }

    // Notify Throughput() when all messages have been received.
    if (this->msgCount >= this->sentMsgs)
    {
      // End the clock.
      this->timeEnd = std::chrono::high_resolution_clock::now();
      condition.notify_all();
    }
  }

  /// \brief Callback that handles latency replies
  /// \param[in] _msg The reply message
  private: void LatencyCb(const gz::msgs::Bytes & /*_msg*/)
  {
    // End the time.
    this->timeEnd = std::chrono::high_resolution_clock::now();

    // Lock and notify
    std::unique_lock<std::mutex> lk(this->mutex);

    this->condition.notify_all();
  }

  /// \brief Create a new message of a give size.
  /// \param[in] _size Size (bytes) of the message to create.
  private: void PrepMsg(const int _size)
  {
    // Prepare the message.
    std::string byteData(_size, '0');
    msg.set_data(byteData);

    // Serialize so that we know how big the message is
    std::string data;
    this->msg.SerializeToString(&data);
    this->dataSize = data.size();
  }

  /// \brief Set of messages sizes to test (bytes).
  private: std::vector<int> msgSizes =
    {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      128000, 256000, 512000, 1000000, 2000000, 4000000
    };

  /// \brief Condition variable used for synchronization.
  private: std::condition_variable condition;

  /// \brief Mutex used for synchronization.
  private: std::mutex mutex;

  /// \brief Message that is sent.
  private: gz::msgs::Bytes msg;

  /// \brief Size of the message currently under test
  private: uint64_t dataSize = 0;

  /// \brief Total bytes received, used for throughput testing
  private: uint64_t totalBytes = 0;

  /// \brief Total messages received, used for throughput testing
  private: uint64_t msgCount = 0;

  /// \brief Number of test iterations.
  private: uint64_t sentMsgs = 100;

  /// \brief Number of warmup iterations per size.
  private: uint64_t warmupIters = 10;

  /// \brief Out-of-order message count (informational).
  private: uint64_t outOfOrder = 0;

  /// \brief Communication node
  private: gz::transport::Node node;

  /// \brief Throughput publisher
  private: gz::transport::Node::Publisher throughputPub;

  /// \brief Latency publisher
  private: gz::transport::Node::Publisher latencyPub;

  /// \brief Used to stop the test.
  private: bool stop = false;

  /// \brief End time point.
  private: std::chrono::time_point<std::chrono::high_resolution_clock> timeEnd;

  /// \brief Output filename or empty string for console output.
  private: std::string filename;

  /// \brief Current iteration count, updated by callbacks or inner loops.
  private: std::atomic<uint64_t> progressIter{0};

  /// \brief True while the progress spinner thread should keep running.
  private: std::atomic<bool> progressRunning{false};

  /// \brief Background spinner thread.
  private: std::thread progressThread;

  /// \brief Timestamp when the current progress interval started.
  private: std::chrono::time_point<std::chrono::steady_clock> progressStart;

  /// \brief Saved display state for the final StopProgress() line.
  private: std::size_t progressSizeIdx{0};
  private: std::size_t progressSizeTotal{0};
  private: uint64_t progressTotal{0};
  private: std::string progressLabel;

  /// \brief CV used to interrupt the progress thread's 200ms sleep.
  private: std::mutex progressWakeMutex;
  private: std::condition_variable progressWakeCv;
};

// The PubTester is global so that the signal handler can easily kill it.
// Ugly, but fine for this example.
PubTester gPubTester;

/// \brief Measures service call latency and throughput using two-way sync
/// calls against a SrvResponder.
///
/// Latency output columns: Test, Size(B), Avg_(us), Min_(us), Max_(us)
///   - Per-call round-trip time divided by 2.
///
/// Throughput output columns: Test, Size(B), RTT-MB/s, Kcalls/s, Loss%
///   - Sequential sync RPC rate; RTT-MB/s counts both req and rep payload.
class SrvTester
{
  public: SrvTester() = default;

  public: void SetOutputFilename(const std::string &_filename)
  {
    this->filename = _filename;
  }

  public: void SetIterations(const uint64_t _iters)
  {
    this->sentMsgs = _iters;
  }

  public: void SetWarmup(const uint64_t _warmup)
  {
    this->warmupIters = _warmup;
  }

  public: void SetShmSizes()
  {
    this->msgSizes = {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      96000, 112000, 120000, 128000, 136000, 144000, 160000, 192000,
      256000, 512000, 1000000, 2000000, 4000000
    };
  }

  public: void Stop()
  {
    this->stop = true;
    this->progressWakeCv.notify_all();
  }

  private: void StartProgress(std::size_t _sizeIdx, std::size_t _sizeTotal,
                               uint64_t _iterTotal,
                               const std::string &_label)
  {
    this->progressSizeIdx = _sizeIdx;
    this->progressSizeTotal = _sizeTotal;
    this->progressTotal = _iterTotal;
    this->progressLabel = _label;
    this->progressIter = 0;
    this->progressStart = std::chrono::steady_clock::now();
    this->progressRunning = true;
    this->progressThread = std::thread(
      [this, _sizeIdx, _sizeTotal, _iterTotal, _label]()
      {
        static const char spin[] = "|/-\\";
        int s = 0;
        while (this->progressRunning.load())
        {
          auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - this->progressStart).count();
          uint64_t it = this->progressIter.load();
          double pct = (_iterTotal > 0)
            ? 100.0 * it / _iterTotal : 0.0;
          std::cerr
            << "\r  [" << _sizeIdx << "/" << _sizeTotal << "] "
            << _label << " " << this->dataSize << "B  "
            << it << "/" << _iterTotal
            << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
            << std::setprecision(1) << ms / 1000.0 << "s  "
            << spin[s++ & 3]
            << "     " << std::flush;
          std::unique_lock<std::mutex> lk(this->progressWakeMutex);
          this->progressWakeCv.wait_for(lk, std::chrono::milliseconds(200),
            [this] { return !this->progressRunning.load(); });
        }
      });
  }

  private: void StopProgress()
  {
    this->progressRunning = false;
    this->progressWakeCv.notify_all();
    if (this->progressThread.joinable())
      this->progressThread.join();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - this->progressStart).count();
    uint64_t it = this->progressIter.load();
    double pct = (this->progressTotal > 0)
      ? 100.0 * it / this->progressTotal : 0.0;
    std::cerr
      << "\r  [" << this->progressSizeIdx << "/" << this->progressSizeTotal
      << "] " << this->progressLabel << " " << this->dataSize << "B  "
      << it << "/" << this->progressTotal
      << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
      << std::setprecision(1) << ms / 1000.0 << "s  done"
      << "     \n" << std::flush;
  }

  private: void OutputHeader(std::ostream *_stream)
  {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);

    (*_stream) << "# " << std::put_time(&tm, "%FT%T%Z") << std::endl;
    (*_stream) << "# Gazebo Transport Version "
               << GZ_TRANSPORT_VERSION_FULL << std::endl;

#ifdef __linux__
    struct utsname unameData;
    uname(&unameData);
    (*_stream) << "# " << unameData.sysname << " " << unameData.release
               << " " << unameData.version << " " << unameData.machine
               << std::endl;
#endif

    const char *impl = std::getenv("GZ_TRANSPORT_IMPLEMENTATION");
    const char *shm = std::getenv("GZ_TRANSPORT_ZENOH_SHM");
    const char *shmPool = std::getenv("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE");
    const char *shmThresh = std::getenv("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD");
    const char *cc = std::getenv("GZ_TRANSPORT_ZENOH_CONGESTION_CONTROL");
    const char *txq = std::getenv("GZ_TRANSPORT_ZENOH_TX_QUEUE_SIZE");

    (*_stream) << "# Backend: " << (impl ? impl : "zeromq (default)")
               << std::endl;
    (*_stream) << "# SHM: " << (shm ? shm : "enabled (default)") << std::endl;
    (*_stream) << "# SHM pool: "
               << (shmPool ? shmPool : "10485760 (default)") << std::endl;
    (*_stream) << "# SHM threshold: "
               << (shmThresh ? shmThresh : "131072 (default)") << std::endl;
    (*_stream) << "# Congestion: " << (cc ? cc : "drop (default)") << std::endl;
    (*_stream) << "# TX queue: " << (txq ? txq : "2 (default)") << std::endl;
    (*_stream) << "# Iterations: " << this->sentMsgs
               << "  Warmup: " << this->warmupIters << std::endl;
  }

  private: void PrepMsg(const int _size)
  {
    std::string byteData(_size, '0');
    this->msg.set_data(byteData);
    std::string data;
    this->msg.SerializeToString(&data);
    this->dataSize = data.size();
  }

  /// \brief Block until /benchmark/service appears in the service list.
  private: void WaitForService()
  {
    while (!this->stop)
    {
      std::vector<std::string> svcs;
      this->node.ServiceList(svcs);
      for (const auto &svc : svcs)
      {
        if (svc == "/benchmark/service")
          return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  /// \brief Measure per-call latency (round-trip / 2) across message sizes.
  public: void Latency()
  {
    this->WaitForService();
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;
    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);
    (*stream) << "# Test\tSize(B)\tAvg_(us)\tMin_(us)\tMax_(us)\n";

    uint64_t maxL = 0;
    uint64_t minL = std::numeric_limits<uint64_t>::max();
    int testNum = 1;

    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      this->PrepMsg(msgSize);

      gz::msgs::Bytes rep;
      bool result;

      // Warmup: prime SHM pool and transport session.
      for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        this->node.Request("/benchmark/service", this->msg, 5000, rep, result);

      uint64_t sum = 0;
      uint64_t received = 0;
      this->StartProgress(testNum, this->msgSizes.size(),
                          this->sentMsgs, "svc-latency");

      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        auto t0 = std::chrono::high_resolution_clock::now();
        bool ok = this->node.Request(
            "/benchmark/service", this->msg, 5000, rep, result);
        auto t1 = std::chrono::high_resolution_clock::now();

        if (ok && result)
        {
          uint64_t d = std::chrono::duration_cast<std::chrono::microseconds>(
              t1 - t0).count();
          sum += d;
          if (d > maxL) maxL = d;
          if (d < minL) minL = d;
          received++;
        }
        this->progressIter = i + 1;
      }

      this->StopProgress();

      // Divide by 2: we measure full round-trip; one-way latency is half.
      double avg = received > 0
        ? (sum / static_cast<double>(received)) * 0.5 : 0.0;
      double minUs = received > 0 ? minL * 0.5 : 0.0;
      double maxUs = received > 0 ? maxL * 0.5 : 0.0;
      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize
                << "\t" << avg << "\t" << minUs << "\t" << maxUs << "\n";
    }
  }

  /// \brief Measure RPC throughput using sequential sync calls.
  public: void Throughput()
  {
    this->WaitForService();
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;
    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);
    (*stream) << "# Test\tSize(B)\t\tRTT-MB/s\tKcalls/s\tLoss%\n";

    int testNum = 1;

    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      this->PrepMsg(msgSize);

      gz::msgs::Bytes rep;
      bool result;

      // Warmup.
      for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        this->node.Request("/benchmark/service", this->msg, 5000, rep, result);

      auto t0 = std::chrono::high_resolution_clock::now();
      uint64_t ok = 0;
      this->StartProgress(testNum, this->msgSizes.size(),
                          this->sentMsgs, "svc-throughput");

      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        if (this->node.Request(
              "/benchmark/service", this->msg, 5000, rep, result) && result)
          ok++;
        this->progressIter = i + 1;
      }

      auto t1 = std::chrono::high_resolution_clock::now();
      this->StopProgress();

      double seconds = std::chrono::duration_cast<std::chrono::microseconds>(
          t1 - t0).count() * 1e-6;
      // RTT-MB/s: both req and rep carry the full payload.
      double mbps = (2.0 * this->dataSize * ok * 1e-6) / seconds;
      double kcallsps = (ok * 1e-3) / seconds;
      double lossPct = this->sentMsgs > 0
        ? std::max(0.0, 100.0 * (1.0 -
            static_cast<double>(ok) / static_cast<double>(this->sentMsgs)))
        : 0.0;

      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize << "\t\t"
                << mbps << "\t" << kcallsps << "\t" << lossPct << "\n";
    }
  }

  private: std::vector<int> msgSizes =
    {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      128000, 256000, 512000, 1000000, 2000000, 4000000
    };

  private: gz::transport::Node node;
  private: gz::msgs::Bytes msg;
  private: uint64_t dataSize = 0;
  private: uint64_t sentMsgs = 100;
  private: uint64_t warmupIters = 10;
  private: bool stop = false;
  private: std::string filename;

  private: std::atomic<uint64_t> progressIter{0};
  private: std::atomic<bool> progressRunning{false};
  private: std::thread progressThread;
  private: std::chrono::time_point<std::chrono::steady_clock> progressStart;
  private: std::size_t progressSizeIdx{0};
  private: std::size_t progressSizeTotal{0};
  private: uint64_t progressTotal{0};
  private: std::string progressLabel;
  private: std::mutex progressWakeMutex;
  private: std::condition_variable progressWakeCv;
};

SrvTester gSrvTester;

/// \brief Measures one-way (fire-and-forget) service call throughput.
///
/// Latency is not applicable since there is no reply. Only Throughput() is
/// provided. MB/s counts request payload only (no reply).
class OneWaySrvTester
{
  public: OneWaySrvTester() = default;

  public: void SetOutputFilename(const std::string &_filename)
  {
    this->filename = _filename;
  }

  public: void SetIterations(const uint64_t _iters)
  {
    this->sentMsgs = _iters;
  }

  public: void SetWarmup(const uint64_t _warmup)
  {
    this->warmupIters = _warmup;
  }

  public: void SetShmSizes()
  {
    this->msgSizes = {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      96000, 112000, 120000, 128000, 136000, 144000, 160000, 192000,
      256000, 512000, 1000000, 2000000, 4000000
    };
  }

  public: void Stop()
  {
    this->stop = true;
    this->progressWakeCv.notify_all();
  }

  private: void StartProgress(std::size_t _sizeIdx, std::size_t _sizeTotal,
                               uint64_t _iterTotal)
  {
    this->progressSizeIdx = _sizeIdx;
    this->progressSizeTotal = _sizeTotal;
    this->progressTotal = _iterTotal;
    this->progressLabel = "oneway";
    this->progressIter = 0;
    this->progressStart = std::chrono::steady_clock::now();
    this->progressRunning = true;
    this->progressThread = std::thread(
      [this, _sizeIdx, _sizeTotal, _iterTotal]()
      {
        static const char spin[] = "|/-\\";
        int s = 0;
        while (this->progressRunning.load())
        {
          auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - this->progressStart).count();
          uint64_t it = this->progressIter.load();
          double pct = (_iterTotal > 0) ? 100.0 * it / _iterTotal : 0.0;
          std::cerr
            << "\r  [" << _sizeIdx << "/" << _sizeTotal << "] "
            << "oneway " << this->dataSize << "B  "
            << it << "/" << _iterTotal
            << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
            << std::setprecision(1) << ms / 1000.0 << "s  "
            << spin[s++ & 3] << "     " << std::flush;
          std::unique_lock<std::mutex> lk(this->progressWakeMutex);
          this->progressWakeCv.wait_for(lk, std::chrono::milliseconds(200),
            [this] { return !this->progressRunning.load(); });
        }
      });
  }

  private: void StopProgress()
  {
    this->progressRunning = false;
    this->progressWakeCv.notify_all();
    if (this->progressThread.joinable())
      this->progressThread.join();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - this->progressStart).count();
    uint64_t it = this->progressIter.load();
    double pct = (this->progressTotal > 0)
      ? 100.0 * it / this->progressTotal : 0.0;
    std::cerr
      << "\r  [" << this->progressSizeIdx << "/" << this->progressSizeTotal
      << "] " << this->progressLabel << " " << this->dataSize << "B  "
      << it << "/" << this->progressTotal
      << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
      << std::setprecision(1) << ms / 1000.0 << "s  done"
      << "     \n" << std::flush;
  }

  private: void OutputHeader(std::ostream *_stream)
  {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    (*_stream) << "# " << std::put_time(&tm, "%FT%T%Z") << std::endl;
    (*_stream) << "# Gazebo Transport Version "
               << GZ_TRANSPORT_VERSION_FULL << std::endl;
#ifdef __linux__
    struct utsname unameData;
    uname(&unameData);
    (*_stream) << "# " << unameData.sysname << " " << unameData.release
               << " " << unameData.version << " " << unameData.machine
               << std::endl;
#endif
    const char *impl = std::getenv("GZ_TRANSPORT_IMPLEMENTATION");
    const char *shm = std::getenv("GZ_TRANSPORT_ZENOH_SHM");
    const char *shmPool = std::getenv("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE");
    const char *shmThresh = std::getenv("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD");
    const char *cc = std::getenv("GZ_TRANSPORT_ZENOH_CONGESTION_CONTROL");
    const char *txq = std::getenv("GZ_TRANSPORT_ZENOH_TX_QUEUE_SIZE");
    (*_stream) << "# Backend: " << (impl ? impl : "zeromq (default)")
               << std::endl;
    (*_stream) << "# SHM: " << (shm ? shm : "enabled (default)") << std::endl;
    (*_stream) << "# SHM pool: "
               << (shmPool ? shmPool : "10485760 (default)") << std::endl;
    (*_stream) << "# SHM threshold: "
               << (shmThresh ? shmThresh : "131072 (default)") << std::endl;
    (*_stream) << "# Congestion: " << (cc ? cc : "drop (default)") << std::endl;
    (*_stream) << "# TX queue: " << (txq ? txq : "2 (default)") << std::endl;
    (*_stream) << "# Iterations: " << this->sentMsgs
               << "  Warmup: " << this->warmupIters << std::endl;
  }

  private: void PrepMsg(const int _size)
  {
    std::string byteData(_size, '0');
    this->msg.set_data(byteData);
    std::string data;
    this->msg.SerializeToString(&data);
    this->dataSize = data.size();
  }

  private: void WaitForService()
  {
    while (!this->stop)
    {
      std::vector<std::string> svcs;
      this->node.ServiceList(svcs);
      for (const auto &svc : svcs)
      {
        if (svc == "/benchmark/oneway")
          return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  /// \brief Measure one-way call send rate.
  ///
  /// Output columns: Test, Size(B), MB/s, Kcalls/s, Loss%
  ///   MB/s counts request payload only (no reply).
  ///   Loss% is calls where node.Request() returned false.
  public: void Throughput()
  {
    this->WaitForService();
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;
    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);
    (*stream) << "# Test\tSize(B)\t\tMB/s\t\tKcalls/s\tLoss%\n";

    int testNum = 1;

    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      this->PrepMsg(msgSize);

      // Warmup: prime SHM pool and service discovery.
      for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        this->node.Request("/benchmark/oneway", this->msg);
      // Brief drain pause so warmup messages don't bleed into measurement.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      auto t0 = std::chrono::high_resolution_clock::now();
      uint64_t ok = 0;
      this->StartProgress(testNum, this->msgSizes.size(), this->sentMsgs);

      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        if (this->node.Request("/benchmark/oneway", this->msg))
          ok++;
        this->progressIter = i + 1;
      }

      auto t1 = std::chrono::high_resolution_clock::now();
      this->StopProgress();

      double seconds = std::chrono::duration_cast<std::chrono::microseconds>(
          t1 - t0).count() * 1e-6;
      double mbps = (this->dataSize * ok * 1e-6) / seconds;
      double kcallsps = (ok * 1e-3) / seconds;
      double lossPct = this->sentMsgs > 0
        ? std::max(0.0, 100.0 * (1.0 -
            static_cast<double>(ok) / static_cast<double>(this->sentMsgs)))
        : 0.0;

      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize << "\t\t"
                << mbps << "\t" << kcallsps << "\t" << lossPct << "\n";
    }
  }

  private: std::vector<int> msgSizes =
    {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      128000, 256000, 512000, 1000000, 2000000, 4000000
    };

  private: gz::transport::Node node;
  private: gz::msgs::Bytes msg;
  private: uint64_t dataSize = 0;
  private: uint64_t sentMsgs = 100;
  private: uint64_t warmupIters = 10;
  private: bool stop = false;
  private: std::string filename;

  private: std::atomic<uint64_t> progressIter{0};
  private: std::atomic<bool> progressRunning{false};
  private: std::thread progressThread;
  private: std::chrono::time_point<std::chrono::steady_clock> progressStart;
  private: std::size_t progressSizeIdx{0};
  private: std::size_t progressSizeTotal{0};
  private: uint64_t progressTotal{0};
  private: std::string progressLabel;
  private: std::mutex progressWakeMutex;
  private: std::condition_variable progressWakeCv;
};

OneWaySrvTester gOneWaySrvTester;

/// \brief Measures no-input (reply-only) service call latency and throughput.
///
/// Before each size test the client sends a configure call to tell the server
/// how many bytes to put in the reply. This makes the reply size comparable
/// to the request size used by the two-way benchmark.
class NoInputSrvTester
{
  public: NoInputSrvTester() = default;

  public: void SetOutputFilename(const std::string &_filename)
  {
    this->filename = _filename;
  }

  public: void SetIterations(const uint64_t _iters)
  {
    this->sentMsgs = _iters;
  }

  public: void SetWarmup(const uint64_t _warmup)
  {
    this->warmupIters = _warmup;
  }

  public: void SetShmSizes()
  {
    this->msgSizes = {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      96000, 112000, 120000, 128000, 136000, 144000, 160000, 192000,
      256000, 512000, 1000000, 2000000, 4000000
    };
  }

  public: void Stop()
  {
    this->stop = true;
    this->progressWakeCv.notify_all();
  }

  private: void StartProgress(std::size_t _sizeIdx, std::size_t _sizeTotal,
                               uint64_t _iterTotal,
                               const std::string &_label)
  {
    this->progressSizeIdx = _sizeIdx;
    this->progressSizeTotal = _sizeTotal;
    this->progressTotal = _iterTotal;
    this->progressLabel = _label;
    this->progressIter = 0;
    this->progressStart = std::chrono::steady_clock::now();
    this->progressRunning = true;
    this->progressThread = std::thread(
      [this, _sizeIdx, _sizeTotal, _iterTotal, _label]()
      {
        static const char spin[] = "|/-\\";
        int s = 0;
        while (this->progressRunning.load())
        {
          auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - this->progressStart).count();
          uint64_t it = this->progressIter.load();
          double pct = (_iterTotal > 0) ? 100.0 * it / _iterTotal : 0.0;
          std::cerr
            << "\r  [" << _sizeIdx << "/" << _sizeTotal << "] "
            << _label << " " << this->dataSize << "B  "
            << it << "/" << _iterTotal
            << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
            << std::setprecision(1) << ms / 1000.0 << "s  "
            << spin[s++ & 3] << "     " << std::flush;
          std::unique_lock<std::mutex> lk(this->progressWakeMutex);
          this->progressWakeCv.wait_for(lk, std::chrono::milliseconds(200),
            [this] { return !this->progressRunning.load(); });
        }
      });
  }

  private: void StopProgress()
  {
    this->progressRunning = false;
    this->progressWakeCv.notify_all();
    if (this->progressThread.joinable())
      this->progressThread.join();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - this->progressStart).count();
    uint64_t it = this->progressIter.load();
    double pct = (this->progressTotal > 0)
      ? 100.0 * it / this->progressTotal : 0.0;
    std::cerr
      << "\r  [" << this->progressSizeIdx << "/" << this->progressSizeTotal
      << "] " << this->progressLabel << " " << this->dataSize << "B  "
      << it << "/" << this->progressTotal
      << " (" << std::fixed << std::setprecision(0) << pct << "%)  "
      << std::setprecision(1) << ms / 1000.0 << "s  done"
      << "     \n" << std::flush;
  }

  private: void OutputHeader(std::ostream *_stream)
  {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    (*_stream) << "# " << std::put_time(&tm, "%FT%T%Z") << std::endl;
    (*_stream) << "# Gazebo Transport Version "
               << GZ_TRANSPORT_VERSION_FULL << std::endl;
#ifdef __linux__
    struct utsname unameData;
    uname(&unameData);
    (*_stream) << "# " << unameData.sysname << " " << unameData.release
               << " " << unameData.version << " " << unameData.machine
               << std::endl;
#endif
    const char *impl = std::getenv("GZ_TRANSPORT_IMPLEMENTATION");
    const char *shm = std::getenv("GZ_TRANSPORT_ZENOH_SHM");
    const char *shmPool = std::getenv("GZ_TRANSPORT_ZENOH_SHM_POOL_SIZE");
    const char *shmThresh = std::getenv("GZ_TRANSPORT_ZENOH_SHM_THRESHOLD");
    const char *cc = std::getenv("GZ_TRANSPORT_ZENOH_CONGESTION_CONTROL");
    const char *txq = std::getenv("GZ_TRANSPORT_ZENOH_TX_QUEUE_SIZE");
    (*_stream) << "# Backend: " << (impl ? impl : "zeromq (default)")
               << std::endl;
    (*_stream) << "# SHM: " << (shm ? shm : "enabled (default)") << std::endl;
    (*_stream) << "# SHM pool: "
               << (shmPool ? shmPool : "10485760 (default)") << std::endl;
    (*_stream) << "# SHM threshold: "
               << (shmThresh ? shmThresh : "131072 (default)") << std::endl;
    (*_stream) << "# Congestion: " << (cc ? cc : "drop (default)") << std::endl;
    (*_stream) << "# TX queue: " << (txq ? txq : "2 (default)") << std::endl;
    (*_stream) << "# Iterations: " << this->sentMsgs
               << "  Warmup: " << this->warmupIters << std::endl;
  }

  /// \brief Compute the serialized reply size for a given raw data length and
  /// configure the server to use that length.
  private: bool ConfigureSize(const int _msgSize)
  {
    gz::msgs::Int32 sizeMsg;
    sizeMsg.set_data(_msgSize);
    gz::msgs::Boolean boolRep;
    bool result;
    if (!this->node.Request(
          "/benchmark/noinput/configure", sizeMsg, 5000, boolRep, result)
        || !result)
    {
      std::cerr << "Failed to configure /benchmark/noinput reply size\n";
      return false;
    }
    // Compute expected dataSize by serialising a Bytes message of _msgSize.
    gz::msgs::Bytes tmp;
    tmp.set_data(std::string(_msgSize, '0'));
    std::string serialised;
    tmp.SerializeToString(&serialised);
    this->dataSize = serialised.size();
    return true;
  }

  private: void WaitForService()
  {
    while (!this->stop)
    {
      std::vector<std::string> svcs;
      this->node.ServiceList(svcs);
      for (const auto &svc : svcs)
      {
        if (svc == "/benchmark/noinput")
          return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  /// \brief Measure per-call latency (round-trip / 2) across reply sizes.
  public: void Latency()
  {
    this->WaitForService();
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;
    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);
    (*stream) << "# Test\tSize(B)\tAvg_(us)\tMin_(us)\tMax_(us)\n";

    uint64_t maxL = 0;
    uint64_t minL = std::numeric_limits<uint64_t>::max();
    int testNum = 1;

    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      if (!this->ConfigureSize(msgSize))
        return;

      gz::msgs::Bytes rep;
      bool result;

      // Warmup.
      for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        this->node.Request("/benchmark/noinput", 5000, rep, result);

      uint64_t sum = 0;
      uint64_t received = 0;
      this->StartProgress(testNum, this->msgSizes.size(),
                          this->sentMsgs, "noinput-latency");

      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        auto t0 = std::chrono::high_resolution_clock::now();
        bool ok = this->node.Request("/benchmark/noinput", 5000, rep, result);
        auto t1 = std::chrono::high_resolution_clock::now();

        if (ok && result)
        {
          uint64_t d = std::chrono::duration_cast<std::chrono::microseconds>(
              t1 - t0).count();
          sum += d;
          if (d > maxL) maxL = d;
          if (d < minL) minL = d;
          received++;
        }
        this->progressIter = i + 1;
      }

      this->StopProgress();

      double avg = received > 0
        ? (sum / static_cast<double>(received)) * 0.5 : 0.0;
      double minUs = received > 0 ? minL * 0.5 : 0.0;
      double maxUs = received > 0 ? maxL * 0.5 : 0.0;
      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize
                << "\t" << avg << "\t" << minUs << "\t" << maxUs << "\n";
    }
  }

  /// \brief Measure RPC throughput (sequential sync no-input calls).
  ///
  /// Output columns: Test, Size(B), MB/s, Kcalls/s, Loss%
  ///   MB/s counts reply payload only (no request).
  public: void Throughput()
  {
    this->WaitForService();
    if (this->stop)
      return;

    std::ostream *stream = &std::cout;
    std::ofstream fstream;
    if (!this->filename.empty())
    {
      fstream.open(this->filename);
      stream = &fstream;
    }

    this->OutputHeader(stream);
    (*stream) << "# Test\tSize(B)\t\tMB/s\t\tKcalls/s\tLoss%\n";

    int testNum = 1;

    for (auto msgSize : this->msgSizes)
    {
      if (this->stop)
        return;

      if (!this->ConfigureSize(msgSize))
        return;

      gz::msgs::Bytes rep;
      bool result;

      // Warmup.
      for (uint64_t i = 0; i < this->warmupIters && !this->stop; ++i)
        this->node.Request("/benchmark/noinput", 5000, rep, result);

      auto t0 = std::chrono::high_resolution_clock::now();
      uint64_t ok = 0;
      this->StartProgress(testNum, this->msgSizes.size(),
                          this->sentMsgs, "noinput-throughput");

      for (uint64_t i = 0; i < this->sentMsgs && !this->stop; ++i)
      {
        if (this->node.Request(
              "/benchmark/noinput", 5000, rep, result) && result)
          ok++;
        this->progressIter = i + 1;
      }

      auto t1 = std::chrono::high_resolution_clock::now();
      this->StopProgress();

      double seconds = std::chrono::duration_cast<std::chrono::microseconds>(
          t1 - t0).count() * 1e-6;
      // MB/s counts reply payload only (no request payload).
      double mbps = (this->dataSize * ok * 1e-6) / seconds;
      double kcallsps = (ok * 1e-3) / seconds;
      double lossPct = this->sentMsgs > 0
        ? std::max(0.0, 100.0 * (1.0 -
            static_cast<double>(ok) / static_cast<double>(this->sentMsgs)))
        : 0.0;

      (*stream) << std::fixed << testNum++ << "\t" << this->dataSize << "\t\t"
                << mbps << "\t" << kcallsps << "\t" << lossPct << "\n";
    }
  }

  private: std::vector<int> msgSizes =
    {
      256, 512, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
      128000, 256000, 512000, 1000000, 2000000, 4000000
    };

  private: gz::transport::Node node;
  private: uint64_t dataSize = 0;
  private: uint64_t sentMsgs = 100;
  private: uint64_t warmupIters = 10;
  private: bool stop = false;
  private: std::string filename;

  private: std::atomic<uint64_t> progressIter{0};
  private: std::atomic<bool> progressRunning{false};
  private: std::thread progressThread;
  private: std::chrono::time_point<std::chrono::steady_clock> progressStart;
  private: std::size_t progressSizeIdx{0};
  private: std::size_t progressSizeTotal{0};
  private: uint64_t progressTotal{0};
  private: std::string progressLabel;
  private: std::mutex progressWakeMutex;
  private: std::condition_variable progressWakeCv;
};

NoInputSrvTester gNoInputSrvTester;

//////////////////////////////////////////////////
void signalHandler(int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
  {
    // Move past the \r-based progress line before anything else prints.
    // write() is async-signal-safe; std::cerr is not.
#ifdef __linux__
    write(STDERR_FILENO, "\n", 1);
#endif
    gStop = true;
    gCondition.notify_all();
    gPubTester.Stop();
    gSrvTester.Stop();
    gOneWaySrvTester.Stop();
    gNoInputSrvTester.Stop();
  }
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Install a signal handler for SIGINT and SIGTERM.
  std::signal(SIGINT,  signalHandler);
  std::signal(SIGTERM, signalHandler);

  // Simple usage.
  std::string usage = "Benchmark testing program."
    " Usage:\n ./bench <options>\n\n"
    " Example intraprocess latency:\n\t./bench -l\n"
    " Example interprocess latency:\n"
    " \tTerminal 1: ./bench -l -r\n"
    " \tTerminal 2: ./bench -l -p\n"
    " Example intraprocess throughput:\n\t./bench -t\n"
    " Example interprocess throughput:\n"
    " \tTerminal 1: ./bench -t -r\n"
    " \tTerminal 2: ./bench -t -p\n"
    " Example SHM benchmark (extended sizes):\n"
    " \tTerminal 1: ./bench -t -r -s\n"
    " \tTerminal 2: ./bench -t -p -s\n"
    " Example two-way service latency (intraprocess):\n\t./bench -l -c\n"
    " Example two-way service latency (interprocess):\n"
    " \tTerminal 1: ./bench -r -c\n"
    " \tTerminal 2: ./bench -l -p -c\n"
    " Example two-way service throughput (intraprocess):\n\t./bench -t -c\n"
    " Example two-way service throughput (interprocess):\n"
    " \tTerminal 1: ./bench -r -c\n"
    " \tTerminal 2: ./bench -t -p -c\n"
    " Example one-way service throughput (intraprocess):\n\t./bench -t --oneway\n"
    " Example one-way service throughput (interprocess):\n"
    " \tTerminal 1: ./bench -r --oneway\n"
    " \tTerminal 2: ./bench -t -p --oneway\n"
    " Example no-input service latency (intraprocess):\n\t./bench -l --noinput\n"
    " Example no-input service throughput (intraprocess):\n\t./bench -t --noinput\n"
    " Example no-input service (interprocess):\n"
    " \tTerminal 1: ./bench -r --noinput\n"
    " \tTerminal 2: ./bench -l -p --noinput\n";

  gflags::SetUsageMessage(usage);

  // Parse command line arguments
  gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);

  // Show help, if specified
  if (FLAGS_h)
  {
    gflags::SetCommandLineOptionWithMode("help", "false",
        gflags::SET_FLAGS_DEFAULT);
    gflags::SetCommandLineOptionWithMode("helpshort", "true",
        gflags::SET_FLAGS_DEFAULT);
  }
  gflags::HandleCommandLineHelpFlags();

  // Validate: at most one service mode flag.
  int svcModeCount = (FLAGS_c ? 1 : 0) + (FLAGS_oneway ? 1 : 0)
                   + (FLAGS_noinput ? 1 : 0);
  if (svcModeCount > 1)
  {
    std::cerr << "Error: -c, --oneway, and --noinput are mutually exclusive.\n";
    return 1;
  }
  if (FLAGS_oneway && FLAGS_l)
  {
    std::cerr << "Error: --oneway has no reply, latency (-l) is not applicable."
              << " Use -t for throughput.\n";
    return 1;
  }

  // Set the number of iterations.
  gPubTester.SetIterations(FLAGS_i);
  gPubTester.SetWarmup(FLAGS_w);
  gPubTester.SetOutputFilename(FLAGS_o);
  gSrvTester.SetIterations(FLAGS_i);
  gSrvTester.SetWarmup(FLAGS_w);
  gSrvTester.SetOutputFilename(FLAGS_o);
  gOneWaySrvTester.SetIterations(FLAGS_i);
  gOneWaySrvTester.SetWarmup(FLAGS_w);
  gOneWaySrvTester.SetOutputFilename(FLAGS_o);
  gNoInputSrvTester.SetIterations(FLAGS_i);
  gNoInputSrvTester.SetWarmup(FLAGS_w);
  gNoInputSrvTester.SetOutputFilename(FLAGS_o);

  // Use extended SHM sizes if requested
  if (FLAGS_s)
  {
    gPubTester.SetShmSizes();
    gSrvTester.SetShmSizes();
    gOneWaySrvTester.SetShmSizes();
    gNoInputSrvTester.SetShmSizes();
  }

  // Run the responder/server
  if (FLAGS_r)
  {
    std::unique_lock<std::mutex> lk(gMutex);
    if (FLAGS_c)
    {
      SrvResponder srvResponder;
      gCondition.wait(lk, []{return gStop;});
    }
    else if (FLAGS_oneway)
    {
      OneWayResponder oneWayResponder;
      gCondition.wait(lk, []{return gStop;});
    }
    else if (FLAGS_noinput)
    {
      NoInputResponder noInputResponder;
      gCondition.wait(lk, []{return gStop;});
    }
    else
    {
      FloodSub floodSub(FLAGS_f);
      ReplyTester replyTester;
      gCondition.wait(lk, []{return gStop;});
    }
  }
  // Run the caller/publisher
  else if (FLAGS_p)
  {
    if (FLAGS_c)
    {
      if (FLAGS_t)
        gSrvTester.Throughput();
      else
        gSrvTester.Latency();
    }
    else if (FLAGS_oneway)
    {
      gOneWaySrvTester.Throughput();
    }
    else if (FLAGS_noinput)
    {
      if (FLAGS_t)
        gNoInputSrvTester.Throughput();
      else
        gNoInputSrvTester.Latency();
    }
    else
    {
      FloodPub floodPub(FLAGS_f);
      gPubTester.Init();
      if (FLAGS_t)
        gPubTester.Throughput();
      else
        gPubTester.Latency();
    }
  }
  // Single process with both server and client
  else
  {
    if (FLAGS_c)
    {
      SrvResponder srvResponder;
      if (FLAGS_t)
        gSrvTester.Throughput();
      else
        gSrvTester.Latency();
    }
    else if (FLAGS_oneway)
    {
      OneWayResponder oneWayResponder;
      gOneWaySrvTester.Throughput();
    }
    else if (FLAGS_noinput)
    {
      NoInputResponder noInputResponder;
      if (FLAGS_t)
        gNoInputSrvTester.Throughput();
      else
        gNoInputSrvTester.Latency();
    }
    else
    {
      ReplyTester replyTester;
      gPubTester.Init();
      if (FLAGS_t)
        gPubTester.Throughput();
      else
        gPubTester.Latency();
    }
  }
  return 0;
}
