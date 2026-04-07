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

#ifndef GZ_TRANSPORT_NODESHAREDPRIVATE_HH_
#define GZ_TRANSPORT_NODESHAREDPRIVATE_HH_

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <zmq.hpp>
#ifdef HAVE_ZENOH
#include <zenoh.hxx>
#endif

#include "gz/transport/config.hh"
#include "gz/transport/Exception.hh"
#include "gz/transport/Node.hh"
#include "Discovery.hh"
#include "ShmHelpers.hh"

namespace gz::transport
{
  // Inline bracket to help doxygen filtering.
  inline namespace GZ_TRANSPORT_VERSION_NAMESPACE {
  //
  /// \brief Metadata for a publication. This is sent as part of the ZMQ
  /// message for topic statistics.
  class PublicationMetadata
  {
    /// \brief Publication timestamp.
    public: uint64_t stamp = 0;

    /// \brief Sequence number, used to detect dropped messages.
    public: uint64_t seq = 0;
  };

  //
  // Private data class for NodeShared.
  class NodeSharedPrivate
  {
    /// \brief Constructor.
    /// \throws gz::transport::Exception if a Zenoh session cannot be opened
    /// (e.g. when using client mode without a reachable router).
    public: NodeSharedPrivate()
    {
      // Determine implementation first, before creating any resources.
      std::string gzImpl;
      if (env("GZ_TRANSPORT_IMPLEMENTATION", gzImpl) && !gzImpl.empty())
      {
        std::transform(gzImpl.begin(), gzImpl.end(), gzImpl.begin(), ::tolower);
        if (gzImpl == "zeromq" || gzImpl == "zenoh")
          this->gzImplementation = gzImpl;
        else
        {
          std::cerr << "Unrecognized value in GZ_TRANSPORT_IMPLEMENTATION. ["
                    << gzImpl << "]. Ignoring this value" << std::endl;
        }
      }

      // Create resources based on implementation.
      if (this->gzImplementation == "zeromq")
      {
        this->context.reset(new zmq::context_t(1));
        this->publisher.reset(new zmq::socket_t(*context, ZMQ_PUB));
        this->subscriber.reset(new zmq::socket_t(*context, ZMQ_SUB));
        this->requester.reset(new zmq::socket_t(*context, ZMQ_ROUTER));
        this->responseReceiver.reset(new zmq::socket_t(*context, ZMQ_ROUTER));
        this->replier.reset(new zmq::socket_t(*context, ZMQ_ROUTER));
      }
#ifdef HAVE_ZENOH
      else if (this->gzImplementation == "zenoh")
      {
        ZenohConfigSource configSource;
        auto config = ZenohConfig(configSource);
        if (this->verbose)
        {
          if (configSource == ZenohConfigSource::kFromEnvVariable)
            std::cout << "Zenoh config loaded from ZENOH_CONFIG" << std::endl;
          else
            std::cout << "Zenoh default config loaded" << std::endl;
        }

        // Increase the congestion control drop timeouts from Zenoh's
        // defaults (1 ms / 50 ms) to reduce message loss for large
        // messages in inter-process pub/sub.  Benchmarking showed that
        // the default 1 ms wait_before_drop causes 50-90% loss for
        // messages in the 125 KB–4 MB range, while 50 ms eliminates
        // loss entirely without measurable throughput impact.
        // These can be overridden via ZENOH_CONFIG or
        // GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE.
        config.insert_json5(
          "transport/link/tx/queue/congestion_control/drop/"
          "wait_before_drop", "50000");
        config.insert_json5(
          "transport/link/tx/queue/congestion_control/drop/"
          "max_wait_before_drop_fragments", "250000");

        // Apply key=value overrides from GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE.
        // Applied after our defaults so user overrides take priority.
        const char *overrideEnv =
            std::getenv("GZ_TRANSPORT_ZENOH_CONFIG_OVERRIDE");
        if (overrideEnv)
          ApplyZenohConfigOverrides(config, overrideEnv, this->verbose);

        // Read the resolved SHM enabled flag from the Zenoh config
        // (after ZENOH_CONFIG file + overrides). This is the single
        // source of truth — users control SHM via Zenoh's native
        // transport/shared_memory/enabled setting.
        {
          auto shmVal = config.get(
            "transport/shared_memory/enabled");
          shmEnvConfig().enabled =
            (shmVal != "false" && shmVal != "0");
        }

        try
        {
          this->session = std::make_shared<zenoh::Session>(
            zenoh::Session::open(std::move(config)));
        }
        catch (const zenoh::ZException &e)
        {
          // Throw rather than continuing with a null session, which
          // would cause segfaults downstream. This can happen when
          // using client mode without a reachable router. Users can
          // configure connect.timeout_ms in the Zenoh config to wait
          // for the router to become available.
          throw gz::transport::Exception(
            std::string("Failed to open Zenoh session: ") + e.what());
        }
      }
#endif
    }

    /// \brief Initialize security
    public: void SecurityInit();

    /// \brief Handle new secure connections
    public: void SecurityOnNewConnection();

    /// \brief Access control handler for plain security.
    /// This function is designed to be run in a thread.
    public: void AccessControlHandler();

    /// \brief Get and validate a non-negative environment variable.
    /// \param[in] _envVar The name of the environment variable to get.
    /// \param[in] _defaultValue The default value returned in case the
    /// environment variable is invalid (e.g.: invalid number,
    /// negative number).
    /// \return The value read from the environment variable or the default
    /// value if the validation wasn't succeed.
    public: int NonNegativeEnvVar(const std::string &_envVar,
                                  int _defaultValue) const;

#ifdef HAVE_ZENOH
    /// \brief Enumeration for the source of Zenoh configuration.
    public: enum class ZenohConfigSource
            {
              /// \brief Configuration loaded from ZENOH_CONFIG env variable.
              kFromEnvVariable,
              /// \brief Default Zenoh configuration.
              kDefault
            };

    /// \brief Get the Zenoh configuration.
    /// If the environment variable ZENOH_CONFIG is set, use that config file.
    /// Otherwise, use the default Zenoh configuration.
    /// \param[out] _configSource The source of the configuration.
    /// \return The Zenoh configuration object.
    public: inline zenoh::Config ZenohConfig(ZenohConfigSource &_configSource)
            {
              const char *zenohConfigEnv = std::getenv("ZENOH_CONFIG");
              if (zenohConfigEnv)
              {
                std::string configPath(zenohConfigEnv);

                // Check if the file exists and is a regular file.
                if (std::filesystem::is_regular_file(configPath))
                {
                  // Try to load the config file.
                  zenoh::ZResult result;
                  zenoh::Config config =
                    zenoh::Config::from_file(configPath, &result);
                  if (result == Z_OK)
                  {
                    _configSource = ZenohConfigSource::kFromEnvVariable;
                    return config;
                  }
                  std::cerr << "Failed to parse Zenoh config file: "
                            << configPath << "\n";
                }
                else
                {
                  std::cerr << "Zenoh config file not found: "
                            << configPath << "\n";
                }
              }

              // Fallback to default configuration.
              _configSource = ZenohConfigSource::kDefault;
              return zenoh::Config::create_default();
            }

    /// \brief Apply key=value config overrides to a Zenoh config.
    /// Format: "key1=value1;key2=value2;..."
    /// Values are passed directly to insert_json5(), so they can be any
    /// valid JSON5 (strings, numbers, objects, arrays).
    /// \param[in,out] _config The Zenoh config to modify.
    /// \param[in] _overrides The override string to parse.
    /// \param[in] _verbose Print applied overrides to stdout.
    public: static inline void ApplyZenohConfigOverrides(
              zenoh::Config &_config,
              const std::string &_overrides,
              bool _verbose = false)
            {
              std::string::size_type pos = 0;
              while (pos < _overrides.size())
              {
                auto semi = _overrides.find(';', pos);
                if (semi == std::string::npos)
                  semi = _overrides.size();
                auto eq = _overrides.find('=', pos);
                if (eq != std::string::npos && eq < semi)
                {
                  std::string key = _overrides.substr(pos, eq - pos);
                  std::string val = _overrides.substr(eq + 1, semi - eq - 1);
                  // Trim leading/trailing whitespace.
                  auto trim = [](std::string &s)
                  {
                    auto start = s.find_first_not_of(" \t");
                    auto end = s.find_last_not_of(" \t");
                    s = (start == std::string::npos) ? "" :
                        s.substr(start, end - start + 1);
                  };
                  trim(key);
                  trim(val);
                  if (!key.empty())
                  {
                    zenoh::ZResult result;
                    _config.insert_json5(key, val, &result);
                    if (result != Z_OK)
                    {
                      std::cerr << "gz-transport: failed to apply Zenoh "
                                << "config override: " << key << "="
                                << val << std::endl;
                    }
                    else if (_verbose)
                    {
                      std::cout << "Zenoh config override: " << key
                                << "=" << val << std::endl;
                    }
                  }
                }
                pos = semi + 1;
              }
            }

    /// \brief Pointer to the Zenoh session.
    public: std::shared_ptr<zenoh::Session> session;

    /// \brief Centralized Zenoh subscribers. One subscriber per topic.
    /// The callback dispatches to all registered handlers via
    /// TriggerCallbacks, ensuring O(1) copy + O(1) deserialization
    /// regardless of how many handlers are registered.
    public: std::map<std::string, std::unique_ptr<zenoh::Subscriber<void>>>
              zenohSubscribers;
#endif

    //////////////////////////////////////////////////
    ///////    Declare here the ZMQ Context    ///////
    //////////////////////////////////////////////////

    /// \brief 0MQ context. Always declare this object before any ZMQ socket
    /// to make sure that the context is destroyed after all sockets.
    public: std::unique_ptr<zmq::context_t> context;

    //////////////////////////////////////////////////
    ///////     Declare here all ZMQ sockets   ///////
    //////////////////////////////////////////////////

    /// \brief ZMQ socket to send topic updates.
    public: std::unique_ptr<zmq::socket_t> publisher;

    /// \brief ZMQ socket to receive topic updates.
    public: std::unique_ptr<zmq::socket_t> subscriber;

    /// \brief ZMQ socket for sending service call requests.
    public: std::unique_ptr<zmq::socket_t> requester;

    /// \brief ZMQ socket for receiving service call responses.
    public: std::unique_ptr<zmq::socket_t> responseReceiver;

    /// \brief ZMQ socket to receive service call requests.
    public: std::unique_ptr<zmq::socket_t> replier;

    /// \brief Thread the handle access control
    public: std::thread accessControlThread;

    //////////////////////////////////////////////////
    /////// Declare here the discovery object  ///////
    //////////////////////////////////////////////////

    /// \brief Discovery service (messages).
    public: std::unique_ptr<MsgDiscovery> msgDiscovery;

    /// \brief Discovery service (services).
    public: std::unique_ptr<SrvDiscovery> srvDiscovery;

    //////////////////////////////////////////////////
    /////// Other private member variables     ///////
    //////////////////////////////////////////////////

    /// \brief When true, the reception thread will finish.
    public: std::atomic<bool> exit = false;

    /// \brief Timeout used for receiving messages (ms.).
    public: inline static const int Timeout = 250;

    ////////////////////////////////////////////////////////////////
    /////// The following is for asynchronous publication of ///////
    /////// messages to local subscribers.                    ///////
    ////////////////////////////////////////////////////////////////

    /// \brief Encapsulates information needed to publish a message. An
    /// instance of this class is pushed onto a publish queue, pubQueue, when
    /// a message is published through Node::Publisher::Publish.
    /// The pubThread processes the pubQueue in the
    /// NodeSharedPrivate::PublishThread function.
    ///
    /// A producer-consumer mechanism is used to send messages so that
    /// Node::Publisher::Publish function does not block while executing
    /// local subscriber callbacks.
    public: struct PublishMsgDetails
            {
              /// \brief All the local subscription handlers.
              public: std::vector<ISubscriptionHandlerPtr> localHandlers;

              /// \brief All the raw handlers.
              public: std::vector<RawSubscriptionHandlerPtr> rawHandlers;

              /// \brief Buffer for the raw handlers.
              public: std::unique_ptr<char[]> sharedBuffer = nullptr;

              /// \brief Msg copy for the local handlers.
              public: std::unique_ptr<ProtoMsg> msgCopy = nullptr;

              /// \brief Message size.
              // cppcheck-suppress unusedStructMember
              public: std::size_t msgSize = 0;

              /// \brief Information about the topic and type.
              public: MessageInfo info;

              /// \brief Publisher's node UUID.
              public: std::string publisherNodeUUID;
            };

    /// \brief Publish thread used to process the pubQueue.
    public: std::thread pubThread;

    /// \brief Mutex to protect the pubThread and pubQueue.
    public: std::mutex pubThreadMutex;

    /// \brief List onto which new messages are pushed. The pubThread
    /// will pop off the messages and send them to local subscribers.
    public: std::list<std::unique_ptr<PublishMsgDetails>> pubQueue;

    /// \brief used to signal when new work is available
    public: std::condition_variable signalNewPub;

    /// \brief Service thread used to process the srvQueue.
    public: std::thread srvThread;

    /// \brief Mutex to protect the srvThread and srvQueue.
    public: std::mutex srvThreadMutex;

    /// \brief List onto which new srv publishers are pushed.
    public: std::list<ServicePublisher> srvQueue;

    /// \brief Used to signal when new work is available.
    public: std::condition_variable signalNewSrv;

    /// \brief Handles local publication of messages on the pubQueue.
    public: void PublishThread();

    /// \brief Topic publication sequence numbers.
    public: std::map<std::string, uint64_t> topicPubSeq;

    /// \brief True if topic statistics have been enabled.
    public: bool topicStatsEnabled = false;

    /// \brief Statistics for a topic. The key in the map is the topic
    /// name and the value contains the topic statistics.
    public: std::map<std::string, TopicStatistics> topicStats;

    /// \brief Set of topics that have statistics enabled.
    public: std::map<std::string,
            std::function<void(const TopicStatistics &_stats)>>
              enabledTopicStatistics;

    /// \brief A map of node UUID and its subscribed topics
    public: std::unordered_map<std::string, std::unordered_set<std::string>>
            topicsSubscribed;

    /// \brief Service call repliers.
    public: HandlerStorage<IRepHandler> repliers;

    /// \brief Pending service call requests.
    public: HandlerStorage<IReqHandler> requests;

    /// \brief Print activity to stdout.
    public: int verbose = false;

    /// \brief My pub/sub address.
    public: std::string myAddress;

    /// \brief My requester service call address.
    public: std::string myRequesterAddress;

    /// \brief My replier service call address.
    public: std::string myReplierAddress;

    /// \brief IP address of this host.
    public: std::string hostAddr;

    /// \brief Underlying middleware implementation.
    /// Supported values are: [zenoh, zeromq].
    public: std::string gzImplementation =
                std::string(GZ_TRANSPORT_DEFAULT_IMPLEMENTATION);
  };
  }
}
#endif
