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

#ifndef __IGN_TRANSPORT_DISCOVERY_HH_INCLUDED__
#define __IGN_TRANSPORT_DISCOVERY_HH_INCLUDED__

#include <uuid/uuid.h>
#include <memory>
#include <mutex>
#include <string>
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class Discovery Discovery.hh
    /// \brief A discovery class.
    class Discovery
    {
      /// \brief Constructor.
      /// \param[in] _procUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode..
      public: Discovery(const uuid_t &_procUuid, bool _verbose = false);

      /// \brief Destructor.
      public: virtual ~Discovery();

      /// \brief Advertise a new topic.
      /// \param[in] _topic Topic to be advertised.
      /// \param[in] _addr ZeroMQ address of the topic's publisher.
      /// \param[in] _ctrl ZeroMQ control address of the topic's publisher.
      public: void Advertise(const std::string &_topic,
        const std::string &_addr, const std::string &_ctrl);

      /// \brief Request discovery information about a topic.
      /// \param[in] _topic Topic requested.
      public: void Discover(const std::string &_topic);

      /// \brief Unadvertise a topic. Broadcast a discovery message that will
      /// cancel all the discovery information for this topic.
      /// \param[in] _topic Topic to be unadvertised.
      public: void Unadvertise(const std::string &_topic);

      /// \brief Get the maximum time allowed without receiving any discovery
      /// information from a node before cancel its entry.
      /// \return The value in milliseconds.
      public: unsigned int GetMaxSilenceInterval();

      /// \brief
      /// \return
      public: unsigned int GetPollingInterval();

      /// \brief
      /// \return
      public: unsigned int GetSubInterval();

      /// \brief
      /// \param[in]
      public: void SetMaxSilenceInterval(unsigned int _ms);

      /// \brief
      /// \param[in]
      public: void SetPollingInterval(unsigned int _ms);

      /// \brief
      /// \param[in]
      public: void SetSubInterval(unsigned int _ms);

      /// \brief
      /// \param[in]
      public: void RegisterDiscoverResp(const transport::DiscResponse &_cb);

      /// \brief
      /// \param[in]
      public: template<class C> void RegisterDiscoverResp(
        void(C::*_cb)(const std::string &, const std::string &,
          const std::string &, const std::string &),
            C* _obj)
      {
        std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

        if (_cb != nullptr)
        {
          this->dataPtr->newDiscoveryEvent =
            std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3, std::placeholders::_4);
        }
      }

      /// \brief
      /// \param[in]
      public: void RegisterDisconnectResp(const transport::DiscResponse &_cb);

      /// \brief
      /// \param[in]
      public: template<class C> void RegisterDisconnectResp(
        void(C::*_cb)(const std::string &, const std::string &,
          const std::string &, const std::string &),
            C* _obj)
      {
        std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

        if (_cb != nullptr)
        {
          this->dataPtr->newDisconnectionEvent =
            std::bind(_cb, _obj, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3, std::placeholders::_4);
        }
      }

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<DiscoveryPrivate> dataPtr;
    };
  }
}
#endif
