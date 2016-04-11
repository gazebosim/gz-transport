/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

//  start
//  advertise(IN publisher) -> void
//  unadvertise(IN topic, IN nUUID) -> void
//  discover(IN topic) -> void


//  Info
//  Publisher

//  NewConnection
//  NewDisconnection

#ifndef __IGN_TRANSPORT_DISCOVERY_HH_INCLUDED__
#define __IGN_TRANSPORT_DISCOVERY_HH_INCLUDED__

#ifdef _WIN32
  // For socket(), connect(), send(), and recv().
  #include <Winsock2.h>
  #include <Ws2def.h>
  #include <Ws2ipdef.h>
  #include <Ws2tcpip.h>
  // Type used for raw data on this platform.
  using raw_type = char;
#else
  // For data types
  #include <sys/types.h>
  // For socket(), connect(), send(), and recv()
  #include <sys/socket.h>
  // For gethostbyname()
  #include <netdb.h>
  // For inet_addr()
  #include <arpa/inet.h>
  // For close()
  #include <unistd.h>
  // For sockaddr_in
  #include <netinet/in.h>
  // Type used for raw data on this platform
  using raw_type = void;
#endif

#ifdef _MSC_VER
  #pragma warning(push, 0)
#endif
#include <zmq.hpp>
#ifdef _WIN32
  #pragma warning(pop)
  // Suppress "decorated name length exceed" warning in STL.
  #pragma warning(disable: 4503)
  // Suppress "depreted API warnings" in WINSOCK.
  #pragma warning(disable: 4996)
#endif

#include <algorithm>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NetUtils.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Discovery Discovery.hh ignition/transport/Discovery.hh
    /// \brief A discovery class that implements a distributed topic discovery
    /// protocol. It uses UDP broadcast for sending/receiving messages and
    /// stores updated topic information. The discovery clients can request
    /// the discovery of a topic or the advertisement of a local topic. The
    /// discovery uses heartbeats to track the state of other peers in the
    /// network. The discovery clients can register callbacks to detect when
    /// new topics are discovered or topics are no longer available.
    template<typename Pub>
    class IGNITION_VISIBLE Discovery
    {
      /// \brief Constructor.
      /// \param[in] _pUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] Multicast UDP port used for discovery traffic.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Discovery(const std::string &_pUuid,
                        const int _port,
                        const bool _verbose = false)
        : port(_port),
          hostAddr(determineHost()),
          hostInterfaces(determineInterfaces()),
          pUuid(_pUuid),
          silenceInterval(DefSilenceInterval),
          activityInterval(DefActivityInterval),
          advertiseInterval(DefAdvertiseInterval),
          heartbeatInterval(DefHeartbeatInterval),
          connectionCb(nullptr),
          disconnectionCb(nullptr),
          verbose(_verbose),
          exit(false),
          enabled(false)
      {
#ifdef _WIN32
        if (!initialized)
        {
          WORD wVersionRequested;
          WSADATA wsaData;

          // Request WinSock v2.0.
          wVersionRequested = MAKEWORD(2, 0);
          // Load WinSock DLL.
          if (WSAStartup(wVersionRequested, &wsaData) != 0)
          {
            std::cerr << "Unable to load WinSock DLL" << std::endl;
            return;
          }

          initialized = true;
        }
#endif

        for (const auto &netIface : this->hostInterfaces)
        {
          auto succeed = this->RegisterNetIface(netIface);

          // If the IP address that we're selecting as the main IP address of
          // the host is invalid, we change it to 127.0.0.1 .
          // This is probably because IGN_IP is set to a wrong value.
          if (netIface == this->hostAddr && !succeed)
          {
            this->RegisterNetIface("127.0.0.1");
            std::cerr << "Did you set the environment variable IGN_IP with a "
                      << "correct IP address? " << std::endl
                      << "  [" << netIface << "] seems an invalid local IP "
                      << "address." << std::endl
                      << "  Using 127.0.0.1 as hostname." << std::endl;
            this->hostAddr = "127.0.0.1";
          }
        }

        // Socket option: SO_REUSEADDR. This options is used only for receiving
        // data. We can reuse the same socket for receiving multicast data from
        // multiple interfaces. We will use the socket at position 0 for
        // receiving data.
        int reuseAddr = 1;
        if (setsockopt(this->sockets.at(0), SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char *>(&reuseAddr), sizeof(reuseAddr)) != 0)
        {
          std::cerr << "Error setting socket option (SO_REUSEADDR)."
                    << std::endl;
          return;
        }

#ifdef SO_REUSEPORT
        // Socket option: SO_REUSEPORT. This options is used only for receiving
        // data. We can reuse the same socket for receiving multicast data from
        // multiple interfaces. We will use the socket at position 0 for
        // receiving data.
        int reusePort = 1;
        if (setsockopt(this->sockets.at(0), SOL_SOCKET, SO_REUSEPORT,
            reinterpret_cast<const char *>(&reusePort), sizeof(reusePort)) != 0)
        {
          std::cerr << "Error setting socket option (SO_REUSEPORT)."
                    << std::endl;
          return;
        }
#endif
        // Bind the first socket to the discovery port.
        sockaddr_in localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        localAddr.sin_port = htons(static_cast<u_short>(this->port));

        if (bind(this->sockets.at(0),
          reinterpret_cast<sockaddr *>(&localAddr), sizeof(sockaddr_in)) < 0)
        {
          std::cerr << "Binding to a local port failed." << std::endl;
          return;
        }

        // Set 'mcastAddr' to the multicast discovery group.
        memset(&this->mcastAddr, 0, sizeof(this->mcastAddr));
        this->mcastAddr.sin_family = AF_INET;
        this->mcastAddr.sin_addr.s_addr =
          inet_addr(this->MulticastGroup.c_str());
        this->mcastAddr.sin_port = htons(static_cast<u_short>(this->port));

        if (this->verbose)
          this->PrintCurrentState();
      }

      /// \brief Destructor.
      public: virtual ~Discovery()
      {
        // Tell the service thread to terminate.
        this->exitMutex.lock();
        this->exit = true;
        this->exitMutex.unlock();

        // Don't join on Windows, because it can hang when this object
        // is destructed on process exit (e.g., when it's a global static).
        // I think that it's due to this bug:
        // https://connect.microsoft.com/VisualStudio/feedback/details/747145/std-thread-join-hangs-if-called-after-main-exits-when-using-vs2012-rc
#ifndef _WIN32
        // Wait for the service threads to finish before exit.
        if (this->threadReception.joinable())
          this->threadReception.join();
#else
        bool exitLoop = false;
        while (!exitLoop)
        {
          std::lock_guard<std::mutex> lock(this->exitMutex);
          {
            if (this->threadReceptionExiting)
            {
              exitLoop = true;
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
#endif

        // Broadcast a BYE message to trigger the remote cancellation of
        // all our advertised topics.
        this->SendMsg(ByeType,
          Publisher("", "", this->pUuid, "", Scope_t::ALL));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Close sockets.
        for (const auto &sock : this->sockets)
        {
#ifdef _WIN32
          closesocket(sock);
#else
          close(sock);
#endif
        }
      }

      /// \brief Start the discovery service. You probably want to register the
      /// callbacks for receiving discovery notifications before starting the
      /// service.
      public: void Start()
      {
        {
          std::lock_guard<std::mutex> lock(this->mutex);

          // The service is already running.
          if (this->enabled)
            return;

          this->enabled = true;
        }

        // Start the thread that receives discovery information.
        this->threadReception =
          std::thread(&Discovery::RunReceptionTask, this);

#ifdef _WIN32
        this->threadReceptionExiting = false;
        this->threadReception.detach();
#endif
      }

      /// \brief Advertise a new message.
      /// \param[in] _publisher Publisher's information to advertise.
      /// \return True if the method succeed or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool Advertise(const Pub &_publisher)
      {
        {
          std::lock_guard<std::mutex> lock(this->mutex);

          if (!this->enabled)
            return false;

          // Add the addressing information (local publisher).
          this->info.AddPublisher(_publisher);
        }

        // Only advertise a message outside this process if the scope
        // is not 'Process'
        if (_publisher.Scope() != Scope_t::PROCESS)
          this->SendMsg(AdvType, _publisher);

        return true;
      }

      /// \brief Request discovery information about a topic.
      /// When using this method, the user might want to use
      /// SetConnectionsCb() and SetDisconnectionCb(), that registers callbacks
      /// that will be executed when the topic address is discovered or when the
      /// node providing the topic is disconnected.
      /// \sa SetConnectionsCb.
      /// \sa SetDisconnectionsCb.
      /// \param[in] _topic Topic name requested.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool Discover(const std::string &_topic)
      {
        DiscoveryCallback<Pub> cb;
        bool found;
        Addresses_M<Pub> addresses;

        {
          std::lock_guard<std::mutex> lock(this->mutex);

          if (!this->enabled)
            return false;

          cb = this->connectionCb;
        }

        Pub pub;
        pub.SetTopic(_topic);
        pub.SetPUuid(this->pUuid);
        pub.SetScope(Scope_t::ALL);

        // Broadcast a discovery request for this service call.
        this->SendMsg(SubType, pub);

        {
          std::lock_guard<std::mutex> lock(this->mutex);
          found = this->info.Publishers(_topic, addresses);
        }

        if (found)
        {
          // I already have information about this topic.
          for (auto &proc : addresses)
          {
            for (auto &node : proc.second)
            {
              if (cb)
              {
                // Execute the user's callback for a service request. Notice
                // that we only execute one callback for preventing receive
                //  multiple service responses for a single request.
                cb(node);
              }
            }
          }
        }

        return true;
      }

      /// \brief Get the discovery information object (messages).
      /// \return Reference to the discovery information object.
      public: const TopicStorage<Pub> &Info() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->info;
      }

      /// \brief Get all the publishers' information known for a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _publishers Publishers requested.
      /// \return True if the topic is found and there is at least one publisher
      public: bool Publishers(const std::string &_topic,
                              Addresses_M<Pub> &_publishers)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->info.Publishers(_topic, _publishers);
      }

      /// \brief Unadvertise a new message. Broadcast a discovery
      /// message that will cancel all the discovery information for the topic
      /// advertised by a specific node.
      /// \param[in] _topic Topic name to be unadvertised.
      /// \param[in] _nUuid Node UUID.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      public: bool Unadvertise(const std::string &_topic,
                               const std::string &_nUuid)
      {
        Pub inf;
        {
          std::lock_guard<std::mutex> lock(this->mutex);

          if (!this->enabled)
            return false;

          // Don't do anything if the topic is not advertised by any of my nodes
          if (!this->info.Publisher(_topic, this->pUuid, _nUuid, inf))
            return true;

          // Remove the topic information.
          this->info.DelPublisherByNode(_topic, this->pUuid, _nUuid);
        }

        // Only unadvertise a message outside this process if the scope
        // is not 'Process'.
        if (inf.Scope() != Scope_t::PROCESS)
          this->SendMsg(UnadvType, inf);

        return true;
      }

      /// \brief Get the IP address of this host.
      /// \return A string with this host's IP address.
      public: std::string HostAddr() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->hostAddr;
      }

      /// \brief The discovery checks the validity of the topic information
      /// every 'activity interval' milliseconds.
      /// \sa SetActivityInterval.
      /// \return The value in milliseconds.
      public: unsigned int ActivityInterval() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->activityInterval;
      }

      /// \brief Each node broadcasts periodic heartbeats to keep its topic
      /// information alive in other nodes. A heartbeat message is sent after
      /// 'heartbeat interval' milliseconds.
      /// \sa SetHeartbeatInterval.
      /// \return The value in milliseconds.
      public: unsigned int HeartbeatInterval() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->heartbeatInterval;
      }

      /// \brief While a topic is being advertised by a node, a beacon is sent
      /// periodically every 'advertise interval' milliseconds.
      /// \sa SetAdvertiseInterval.
      /// \return The value in milliseconds.
      public: unsigned int AdvertiseInterval() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->advertiseInterval;
      }

      /// \brief Get the maximum time allowed without receiving any discovery
      /// information from a node before canceling its entries.
      /// \sa SetSilenceInterval.
      /// \return The value in milliseconds.
      public: unsigned int SilenceInterval() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        return this->silenceInterval;
      }

      /// \brief Set the activity interval.
      /// \sa ActivityInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetActivityInterval(const unsigned int _ms)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->activityInterval = _ms;
      }

      /// \brief Set the heartbeat interval.
      /// \sa HeartbeatInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetHeartbeatInterval(const unsigned int _ms)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->heartbeatInterval = _ms;
      }

      /// \brief Set the advertise interval.
      /// \sa AdvertiseInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetAdvertiseInterval(const unsigned int _ms)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->advertiseInterval = _ms;
      }

      /// \brief Set the maximum silence interval.
      /// \sa SilenceInterval.
      /// \param[in] _ms New value in milliseconds.
      public: void SetSilenceInterval(const unsigned int _ms)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->silenceInterval = _ms;
      }

      /// \brief Register a callback to receive discovery connection events.
      /// Each time a new topic is connected, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void ConnectionsCb(const DiscoveryCallback<Pub> &_cb)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->connectionCb = _cb;
      }

      /// \brief Register a callback to receive discovery disconnection events.
      /// Each time a topic is no longer active, the callback will be executed.
      /// This version uses a free function as callback.
      /// \param[in] _cb Function callback.
      public: void DisconnectionsCb(const DiscoveryCallback<Pub> &_cb)
      {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->disconnectionCb = _cb;
      }

      /// \brief Print the current discovery state (info, activity, unknown).
      public: void PrintCurrentState() const
      {
        std::lock_guard<std::mutex> lock(this->mutex);

        std::cout << "---------------" << std::endl;
        std::cout << std::boolalpha << "Enabled: "
                  << this->enabled << std::endl;
        std::cout << "Discovery state" << std::endl;
        std::cout << "\tUUID: " << this->pUuid << std::endl;
        std::cout << "Settings" << std::endl;
        std::cout << "\tActivity: " << this->activityInterval
                  << " ms." << std::endl;
        std::cout << "\tHeartbeat: " << this->heartbeatInterval
                  << "ms." << std::endl;
        std::cout << "\tRetrans.: " << this->advertiseInterval
                  << " ms." << std::endl;
        std::cout << "\tSilence: " << this->silenceInterval
                  << " ms." << std::endl;
        std::cout << "Known information: " << std::endl;
        this->info.Print();

        // Used to calculate the elapsed time.
        Timestamp now = std::chrono::steady_clock::now();

        std::cout << "Activity" << std::endl;
        if (this->activity.empty())
          std::cout << "\t<empty>" << std::endl;
        else
        {
          for (auto &proc : this->activity)
          {
            // Elapsed time since the last update from this publisher.
            std::chrono::duration<double> elapsed = now - proc.second;

            std::cout << "\t" << proc.first << std::endl;
            std::cout << "\t\t" << "Since: " << std::chrono::duration_cast<
              std::chrono::milliseconds>(elapsed).count() << " ms. ago. "
              << std::endl;
          }
        }
        std::cout << "---------------" << std::endl;
      }

      /// \brief Get the list of topics currently advertised in the network.
      /// \param[out] _topics List of advertised topics.
      public: void TopicList(std::vector<std::string> &_topics) const
      {
        this->WaitForInit();
        std::lock_guard<std::mutex> lock(this->mutex);
        this->info.TopicList(_topics);
      }

      /// \brief Get mutex used in the Discovery class.
      /// \return The discovery mutex.
      public: std::mutex& Mutex() const
      {
        return this->mutex;
      }

      /// \brief Check if ready/initialized. If not, then wait on the
      /// initializedCv condition variable.
      public: void WaitForInit() const
      {
        //  bool ready;
        //  {
        //    std::lock_guard<std::mutex> lock(this->mutex);
        //    ready = this->initialized;
        //  }
//
        //  if (!ready)
        //  {
        //    std::unique_lock<std::mutex> lk(this->mutex);
        //    this->initializedCv.wait(
        //       lk, [this]{return this->initialized;});
        //  }
      }

      /// \brief Check the validity of the topic information. Each topic update
      /// has its own timestamp. This method iterates over the list of topics
      /// and invalids the old topics.
      private: void RunActivityTask()
      {
        Timestamp now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(this->mutex);

        std::chrono::duration<double> elapsed = now - this->timeLastActivity;
        if (std::chrono::duration_cast<std::chrono::milliseconds>
           (elapsed).count() < this->activityInterval)
        {
          return;
        }

        for (auto it = this->activity.cbegin(); it != this->activity.cend();)
        {
          // Elapsed time since the last update from this publisher.
          elapsed = now - it->second;

          // This publisher has expired.
          if (std::chrono::duration_cast<std::chrono::milliseconds>
               (elapsed).count() > this->silenceInterval)
          {
            // Remove all the info entries for this process UUID.
            this->info.DelPublishersByProc(it->first);

            // Notify without topic information. This is useful to inform the
            //  client that a remote node is gone, even if we were not
            // interested in its topics.
            Pub publisher;
            publisher.SetPUuid(it->first);
            publisher.SetScope(Scope_t::ALL);
            this->disconnectionCb(publisher);

            // Remove the activity entry.
            this->activity.erase(it++);
          }
          else
            ++it;
        }

        this->timeLastActivity = std::chrono::steady_clock::now();
      }

      /// \brief Broadcast periodic heartbeats.
      private: void RunHeartbeatTask()
      {
        Timestamp now = std::chrono::steady_clock::now();

        {
          std::lock_guard<std::mutex> lock(this->mutex);

          std::chrono::duration<double> elapsed = now - this->timeLastHeartbeat;
          if (std::chrono::duration_cast<std::chrono::milliseconds>
             (elapsed).count() < this->heartbeatInterval)
          {
            return;
          }
        }

        Publisher pub("", "", this->pUuid, "", Scope_t::ALL);
        this->SendMsg(HeartbeatType, pub);

        std::map<std::string, std::vector<Pub>> nodes;
        {
          std::lock_guard<std::mutex> lock(this->mutex);

          // Re-advertise topics that are advertised inside this process.
          this->info.PublishersByProc(this->pUuid, nodes);
        }

        for (const auto &topic : nodes)
        {
          for (const auto &node : topic.second)
            this->SendMsg(AdvType, node);
        }

        {
          std::lock_guard<std::mutex> lock(this->mutex);
          if (!this->initialized)
          {
            ++this->numHeartbeatsUninitialized;
            if (this->numHeartbeatsUninitialized == 2)
            {
              // We consider the discovery initialized after two cycles of
              // heartbeats sent.
              this->initialized = true;

              // Notify anyone waiting for the initialization phase to finish.
              this->initializedCv.notify_all();
            }
          }

          this->timeLastHeartbeat = std::chrono::steady_clock::now();
        }
      }

      /// \brief Receive discovery messages.
      private: void RunReceptionTask()
      {
        bool timeToExit = false;
        while (!timeToExit)
        {
          // Poll socket for a reply, with timeout.
          zmq::pollitem_t items[] =
          {
            {0, this->sockets.at(0), ZMQ_POLLIN, 0},
          };

          // Calculate the timeout.
          auto now = std::chrono::steady_clock::now();
          auto timeUntilNextHeartbeat = (this->timeLastHeartbeat +
            std::chrono::milliseconds(this->heartbeatInterval)) - now;
          auto timeUntilNextActivity = (this->timeLastActivity +
            std::chrono::milliseconds(this->activityInterval)) - now;
          auto timeUntilNextReception = (now +
            std::chrono::milliseconds(this->Timeout)) - now;

          auto t = std::min(timeUntilNextActivity, timeUntilNextActivity);
          auto t2 = std::min(t, timeUntilNextReception);
          int timeout = std::chrono::duration_cast<std::chrono::milliseconds>
            (t2).count();
          timeout = std::max(timeout, 0);

          try
          {
            zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), timeout);
          }
          catch(...)
          {
            continue;
          }

          //  If we got a reply, process it.
          if (items[0].revents & ZMQ_POLLIN)
          {
            this->RecvDiscoveryUpdate();

            if (this->verbose)
              this->PrintCurrentState();
          }

          this->RunHeartbeatTask();
          this->RunActivityTask();

          // Is it time to exit?
          {
            std::lock_guard<std::mutex> lock(this->exitMutex);
            if (this->exit)
              timeToExit = true;
          }
        }
#ifdef _WIN32
        std::lock_guard<std::mutex> lock(this->exitMutex);
        this->threadReceptionExiting = true;
#endif
      }

      /// \brief Method in charge of receiving the discovery updates.
      private: void RecvDiscoveryUpdate()
      {
        char rcvStr[this->MaxRcvStr];
        std::string srcAddr;
        uint16_t srcPort;
        sockaddr_in clntAddr;
        socklen_t addrLen = sizeof(clntAddr);

        if ((recvfrom(this->sockets.at(0),
              reinterpret_cast<raw_type *>(rcvStr),
              this->MaxRcvStr, 0,
              reinterpret_cast<sockaddr *>(&clntAddr),
              reinterpret_cast<socklen_t *>(&addrLen))) < 0)
        {
          std::cerr << "Discovery::RecvDiscoveryUpdate() recvfrom error"
                    << std::endl;
          return;
        }
        srcAddr = inet_ntoa(clntAddr.sin_addr);
        srcPort = ntohs(clntAddr.sin_port);

        if (this->verbose)
        {
          std::cout << "\nReceived discovery update from " << srcAddr << ": "
                    << srcPort << std::endl;
        }

        this->DispatchDiscoveryMsg(srcAddr, rcvStr);
      }


      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _fromIp IP address of the message sender.
      /// \param[in] _msg Received message.
      private: void DispatchDiscoveryMsg(const std::string &_fromIp,
                                         char *_msg)
      {
        Header header;
        char *pBody = _msg;

        // Create the header from the raw bytes.
        header.Unpack(_msg);
        pBody += header.HeaderLength();

        // Discard the message if the wire protocol is different than mine.
        if (this->WireVersion != header.Version())
          return;

        auto recvPUuid = header.PUuid();

        // Discard our own discovery messages.
        if (recvPUuid == this->pUuid)
          return;

        // Update timestamp and cache the callbacks.
        DiscoveryCallback<Pub> connectCb;
        DiscoveryCallback<Pub> disconnectCb;
        {
          std::lock_guard<std::mutex> lock(this->mutex);
          this->activity[recvPUuid] = std::chrono::steady_clock::now();
          connectCb = this->connectionCb;
          disconnectCb = this->disconnectionCb;
        }

        switch (header.Type())
        {
          case AdvType:
          {
            // Read the rest of the fields.
            transport::AdvertiseMessage<Pub> advMsg;
            advMsg.Unpack(pBody);

            // Check scope of the topic.
            if ((advMsg.Publisher().Scope() == Scope_t::PROCESS) ||
                (advMsg.Publisher().Scope() == Scope_t::HOST &&
                 _fromIp != this->hostAddr))
            {
              return;
            }

            // Register an advertised address for the topic.
            bool added;
            {
              std::lock_guard<std::mutex> lock(this->mutex);
              added = this->info.AddPublisher(advMsg.Publisher());
            }

            if (added && connectCb)
            {
              // Execute the client's callback.
              connectCb(advMsg.Publisher());
            }

            break;
          }
          case SubType:
          {
            // Read the rest of the fields.
            SubscriptionMsg subMsg;
            subMsg.Unpack(pBody);
            auto recvTopic = subMsg.Topic();

            // Check if at least one of my nodes advertises the topic requested.
            Addresses_M<Pub> addresses;
            {
              std::lock_guard<std::mutex> lock(this->mutex);
              if (!this->info.HasAnyPublishers(recvTopic, this->pUuid))
              {
                break;
              }

              if (!this->info.Publishers(recvTopic, addresses))
                break;
            }

            for (auto nodeInfo : addresses[this->pUuid])
            {
              // Check scope of the topic.
              if ((nodeInfo.Scope() == Scope_t::PROCESS) ||
                  (nodeInfo.Scope() == Scope_t::HOST &&
                   _fromIp != this->hostAddr))
              {
                continue;
              }

              // Answer an ADVERTISE message.
              this->SendMsg(AdvType, nodeInfo);
            }

            break;
          }
          case HeartbeatType:
          {
            // The timestamp has already been updated.
            break;
          }
          case ByeType:
          {
            // Remove the activity entry for this publisher.
            {
              std::lock_guard<std::mutex> lock(this->mutex);
              this->activity.erase(recvPUuid);
            }

            if (disconnectCb)
            {
              Pub pub;
              pub.SetPUuid(recvPUuid);
              pub.SetScope(Scope_t::ALL);
              // Notify the new disconnection.
              disconnectCb(pub);
            }

            // Remove the address entry for this topic.
            {
              std::lock_guard<std::mutex> lock(this->mutex);
              this->info.DelPublishersByProc(recvPUuid);
            }

            break;
          }
          case UnadvType:
          {
            // Read the address.
            transport::AdvertiseMessage<Pub> advMsg;
            advMsg.Unpack(pBody);

            // Check scope of the topic.
            if ((advMsg.Publisher().Scope() == Scope_t::PROCESS) ||
                (advMsg.Publisher().Scope() == Scope_t::HOST &&
                 _fromIp != this->hostAddr))
            {
              return;
            }

            if (disconnectCb)
            {
              // Notify the new disconnection.
              disconnectCb(advMsg.Publisher());
            }

            // Remove the address entry for this topic.
            {
              std::lock_guard<std::mutex> lock(this->mutex);
              this->info.DelPublisherByNode(advMsg.Publisher().Topic(),
                advMsg.Publisher().PUuid(), advMsg.Publisher().NUuid());
            }

            break;
          }
          default:
          {
            std::cerr << "Unknown message type [" << header.Type() << "]\n";
            break;
          }
        }
      }

      /// \brief Broadcast a discovery message.
      /// \param[in] _type Message type.
      /// \param[in] _pub Publishers's information to send.
      /// \param[in] _flags Optional flags. Currently, the flags are not used
      /// but they will in the future for specifying things like compression,
      /// or encryption.
      private: template<typename T>
      void SendMsg(uint8_t _type,
                   const T &_pub,
                   const uint16_t _flags = 0) const
      {
        // Create the header.
        Header header(this->Version(), _pub.PUuid(), _type, _flags);
        auto msgLength = 0;
        std::vector<char> buffer;

        std::string topic = _pub.Topic();

        switch (_type)
        {
          case AdvType:
          case UnadvType:
          {
            // Create the [UN]ADVERTISE message.
            transport::AdvertiseMessage<T> advMsg(header, _pub);

            // Allocate a buffer and serialize the message.
            buffer.resize(advMsg.MsgLength());
            advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = static_cast<int>(advMsg.MsgLength());
            break;
          }
          case SubType:
          case SubSrvType:
          {
            // Create the [UN]SUBSCRIBE message.
            SubscriptionMsg subMsg(header, topic);

            // Allocate a buffer and serialize the message.
            buffer.resize(subMsg.MsgLength());
            subMsg.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = static_cast<int>(subMsg.MsgLength());
            break;
          }
          case HeartbeatType:
          case ByeType:
          {
            // Allocate a buffer and serialize the message.
            buffer.resize(header.HeaderLength());
            header.Pack(reinterpret_cast<char*>(&buffer[0]));
            msgLength = header.HeaderLength();
            break;
          }
          default:
            std::cerr << "Discovery::SendMsg() error: Unrecognized message"
                      << " type [" << _type << "]" << std::endl;
            return;
        }

        // Send the discovery message to the multicast group through all the
        // sockets.
        for (const auto &sock : this->Sockets())
        {
          if (sendto(sock, reinterpret_cast<const raw_type *>(
            reinterpret_cast<unsigned char*>(&buffer[0])),
            msgLength, 0,
            reinterpret_cast<const sockaddr *>(this->MulticastAddr()),
            sizeof(*(this->MulticastAddr()))) != msgLength)
          {
            std::cerr << "Exception sending a message" << std::endl;
            return;
          }
        }

        if (this->Verbose())
        {
          std::cout << "\t* Sending " << MsgTypesStr[_type]
                    << " msg [" << topic << "]" << std::endl;
        }
      }

      /// \brief Get the list of sockets used for discovery.
      /// \return The list of sockets.
      private: const std::vector<int>& Sockets() const
      {
        return this->sockets;
      }

      /// \brief Get the data structure used for multicast communication.
      /// \return The data structure containing the multicast information.
      private: const sockaddr_in* MulticastAddr() const
      {
        return &this->mcastAddr;
      }

      /// \brief Get the verbose mode.
      /// \return True when verbose mode is enabled or false otherwise.
      private: bool Verbose() const
      {
        return this->verbose;
      }

      /// \brief Get the discovery protocol version.
      /// \return The discovery version.
      private: uint8_t Version() const
      {
        return this->WireVersion;
      }

      /// \brief Register a new network interface in the discovery system.
      /// \param[in] _ip IP address to register.
      /// \return True when the interface was successfully registered or false
      /// otherwise (e.g.: invalid IP address).
      private: bool RegisterNetIface(const std::string &_ip)
      {
        // Make a new socket for sending discovery information.
        int sock = static_cast<int>(socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP));
        if (sock < 0)
        {
          std::cerr << "Socket creation failed." << std::endl;
          return false;
        }

        // Socket option: IP_MULTICAST_IF.
        // This socket option needs to be applied to each socket used to send
        // data. This option selects the source interface for outgoing messages.
        struct in_addr ifAddr;
        ifAddr.s_addr = inet_addr(_ip.c_str());
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
          reinterpret_cast<const char*>(&ifAddr), sizeof(ifAddr)) != 0)
        {
          std::cerr << "Error setting socket option (IP_MULTICAST_IF)."
                    << std::endl;
          return false;
        }

        this->sockets.push_back(sock);

        // Join the multicast group. We have to do it for each network interface
        // but we can do it on the same socket. We will use the socket at
        // position 0 for receiving multicast information.
        struct ip_mreq group;
        group.imr_multiaddr.s_addr =
          inet_addr(this->MulticastGroup.c_str());
        group.imr_interface.s_addr = inet_addr(_ip.c_str());
        if (setsockopt(this->sockets.at(0), IPPROTO_IP, IP_ADD_MEMBERSHIP,
          reinterpret_cast<const char*>(&group), sizeof(group)) != 0)
        {
          std::cerr << "Error setting socket option (IP_ADD_MEMBERSHIP)."
                    << std::endl;
          return false;
        }

        return true;
      }

      /// \brief Default activity interval value (ms.).
      /// \sa ActivityInterval.
      /// \sa SetActivityInterval.
      private: static const unsigned int DefActivityInterval = 100;

      /// \brief Default heartbeat interval value (ms.).
      /// \sa HeartbeatInterval.
      /// \sa SetHeartbeatInterval.
      private: static const unsigned int DefHeartbeatInterval = 1000;

      /// \brief Default silence interval value (ms.).
      /// \sa MaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      private: static const unsigned int DefSilenceInterval = 3000;

      /// \brief Default advertise interval value (ms.).
      /// \sa AdvertiseInterval.
      /// \sa SetAdvertiseInterval.
      private: static const unsigned int DefAdvertiseInterval = 1000;

      /// \brief IP Address used for multicast.
      private: const std::string MulticastGroup = "224.0.0.7";

      /// \brief Timeout used for receiving messages (ms.).
      private: const int Timeout = 250;

      /// \brief Longest string to receive.
      private: static const int MaxRcvStr = 65536;

      /// \brief Wire protocol version. Bump up the version number if you modify
      /// the wire protocol (for discovery or message/service exchange).
      private: static const uint8_t WireVersion = 3;

      /// \brief Port used to broadcast the discovery messages.
      private: int port;

      /// \brief Host IP address.
      private: std::string hostAddr;

      /// \brief List of host network interfaces.
      private: std::vector<std::string> hostInterfaces;

      /// \brief Process UUID.
      private: std::string pUuid;

      /// \brief Silence interval value (ms.).
      /// \sa MaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      private: unsigned int silenceInterval;

      /// \brief Activity interval value (ms.).
      /// \sa ActivityInterval.
      /// \sa SetActivityInterval.
      private: unsigned int activityInterval;

      /// \brief Advertise interval value (ms.).
      /// \sa AdvertiseInterval.
      /// \sa SetAdvertiseInterval.
      private: unsigned int advertiseInterval;

      /// \brief Heartbeat interval value (ms.).
      /// \sa HeartbeatInterval.
      /// \sa SetHeartbeatInterval.
      private: unsigned int heartbeatInterval;

      /// \brief Callback executed when new topics are discovered.
      private: DiscoveryCallback<Pub> connectionCb;

      /// \brief Callback executed when new topics are invalid.
      private: DiscoveryCallback<Pub> disconnectionCb;

      /// \brief Addressing information.
      private: TopicStorage<Pub> info;

      /// \brief Activity information. Every time there is a message from a
      /// remote node, its activity information is updated. If we do not hear
      /// from a node in a while, its entries in 'info' will be invalided. The
      /// key is the process uuid.
      protected: std::map<std::string, Timestamp> activity;

      /// \brief Print discovery information to stdout.
      private: bool verbose;

      /// \brief UDP socket used for sending/receiving discovery messages.
      private: std::vector<int> sockets;

      /// \brief Internet socket address for sending to the multicast group.
      private: sockaddr_in mcastAddr;

      /// \brief Mutex to guarantee exclusive access between the threads.
      private: mutable std::mutex mutex;

      /// \brief Thread in charge of receiving and handling incoming messages.
      private: std::thread threadReception;

      /// \brief Time in which the last heartbeat was sent.
      private: Timestamp timeLastHeartbeat;

      /// \brief Time in which the last activity check was done.
      private: Timestamp timeLastActivity;

      /// \brief Mutex to guarantee exclusive access to the exit variable.
      private: std::mutex exitMutex;

      /// \brief Once the discovery starts, it can take up to
      /// HeartbeatInterval milliseconds to discover the existing nodes on the
      /// network. This variable is 'false' during the first HeartbeatInterval
      /// period and is set to 'true' after that.
      private: bool initialized;

      /// \brief Number of heartbeats sent while discovery is uninitialized.
      private: unsigned int numHeartbeatsUninitialized = 0;

      /// \brief Used to block/unblock until the initialization phase finishes.
      private: std::condition_variable initializedCv;

      /// \brief When true, the service threads will finish.
      private: bool exit;

#ifdef _WIN32
      /// \brief True when the reception thread is finishing.
      private: bool threadReceptionExiting = true;
#endif

      /// \brief When true, the service is enabled.
      private: bool enabled;
    };

    using MsgDiscovery = Discovery<MessagePublisher>;
    using SrvDiscovery = Discovery<ServicePublisher>;
  }
}

#endif
