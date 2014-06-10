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

#ifndef __IGN_TRANSPORT_REQSTORAGE_HH_INCLUDED__
#define __IGN_TRANSPORT_REQSTORAGE_HH_INCLUDED__

#include <map>
#include <string>
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class ReqStorage ReqStorage.hh
    /// \brief Class to store the list of service call requests.
    class ReqStorage
    {
      /// \brief Constructor.
      public: ReqStorage() = default;

      /// \brief Destructor.
      public: virtual ~ReqStorage() = default;

      /// \brief Get the requests handlers for a topic. A request
      /// handler stores the callback and types associated to a service call
      /// request.
      /// \param[in] _topic Topic name.
      /// \param[out] _handlers Request handlers.
      /// \return true if the topic contains at least one request.
      public: bool GetHandlers(const std::string &_topic,
                               IReqHandler_M &_handlers);

      /// \brief Get a specific handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node UUID of the handler.
      /// \param[in] _reqUuid Request UUID.
      /// \param[out] _handlers Handler requested.
      /// \return true if the handler was found.
      public: bool GetHandler(const std::string &_topic,
                              const std::string &_nUuid,
                              const std::string &_reqUuid,
                              IReqHandlerPtr &_handler);

      /// \brief Add a request handler to a topic. A request handler stores
      /// the callback and types associated to a service call request.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \param[in] _handler Request handler.
      public: void AddHandler(const std::string &_topic,
                              const std::string &_nUuid,
                              const IReqHandlerPtr &_handler);

      /// \brief Return true if we have stored at least one request for the
      /// topic.
      /// \param[in] _topic Topic name.
      /// \return true if we have stored at least one request for the topic.
      public: bool HasHandlerForTopic(const std::string &_topic);

      /// \brief Remove a request handler. The node's uuid is used as a key to
      /// remove the appropriate request handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \param[in] _reqUuid Request's UUID to remove.
      public: void RemoveHandler(const std::string &_topic,
                                 const std::string &_nUuid,
                                 const std::string &_reqUuid);

      /// \brief Remove all the handlers from a given node.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      public: void RemoveHandlersForNode(const std::string &_topic,
                                         const std::string &_nUuid);

      /// \brief Stores all the service call requests for each topic.
      private: std::map<std::string, IReqHandler_M> requests;
    };
  }
}

#endif
