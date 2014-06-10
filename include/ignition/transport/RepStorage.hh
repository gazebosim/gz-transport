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

#ifndef __IGN_TRANSPORT_REPSTORAGE_HH_INCLUDED__
#define __IGN_TRANSPORT_REPSTORAGE_HH_INCLUDED__

#include <map>
#include <string>
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class RepStorage RepStorage.hh
    /// \brief Class to store and manage the service call response handlers.
    class RepStorage
    {
      /// \brief Constructor.
      public: RepStorage() = default;

      /// \brief Destructor.
      public: virtual ~RepStorage() = default;

      /// \brief Get the service response handlers for a topic. A response
      /// handler stores the callback and types associated to a responser
      /// service call.
      /// \param[in] _topic Topic name.
      /// \param[out] _handlers Response handlers.
      /// \return true if the topic contains at least one responser.
      public: bool GetHandlers(const std::string &_topic,
                               IRepHandler_M &_handlers);

      /// \brief Add a response handler to a topic. A response handler
      /// stores the callback and types associated to a responser service call.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \param[in] _handler Response handler.
      public: void AddHandler(const std::string &_topic,
                              const std::string &_nUuid,
                              const IRepHandlerPtr &_handler);

      /// \brief Return true if we have stored at least one responser for the
      /// topic.
      /// \param[in] _topic Topic name.
      /// \return true if we have stored at least one responser for the topic.
      public: bool HasHandlerForTopic(const std::string &_topic);

      /// \brief Check if a topic has a response handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \return true if the topic has a response handler registered.
      public: bool HasHandlerForNode(const std::string &_topic,
                                     const std::string &_nUuid);

      /// \brief Remove a response handler. The node's uuid
      /// is used as a key to remove the appropriate response handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      public: void RemoveHandlersForNode(const std::string &_topic,
                                         const std::string &_nUuid);

      /// \brief Stores all the responsers' information for each topic.
      private: std::map<std::string, IRepHandler_M> responsers;
    };
  }
}

#endif
