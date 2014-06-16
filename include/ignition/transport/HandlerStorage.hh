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

#ifndef __IGN_TRANSPORT_HANDLERSTORAGE_HH_INCLUDED__
#define __IGN_TRANSPORT_HANDLERSTORAGE_HH_INCLUDED__

#include <map>
#include <string>
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class HandlerStorage HandlerStorage.hh
    /// \brief Class to store and manage service call handlers.
    template<typename T> class HandlerStorage
    {
      /// \brief Constructor.
      public: HandlerStorage() = default;

      /// \brief Destructor.
      public: virtual ~HandlerStorage() = default;

      /// \brief Get the data handlers for a topic. A request
      /// handler stores the callback and types associated to a service call
      /// request.
      /// \param[in] _topic Topic name.
      /// \param[out] _handlers Request handlers.
      /// \return true if the topic contains at least one request.
      public: bool GetHandlers(const std::string &_topic,
        std::map<std::string,
          std::map<std::string, std::shared_ptr<T> >> &_handlers)
      {
        if (this->data.find(_topic) == this->data.end())
          return false;

        _handlers = this->data[_topic];
        return true;
      }

      /// \brief Get a handler for a topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _handler handler.
      /// \return true if a handler was found.
      public: bool GetHandler(const std::string &_topic,
        std::shared_ptr<T> &_handler)
      {
        if (this->data.find(_topic) == this->data.end())
          return false;

        _handler = this->data[_topic].begin()->second.begin()->second;
        return true;
      }

      /// \brief Get a specific handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node UUID of the handler.
      /// \param[in] _reqUuid Request UUID.
      /// \param[out] _handlers Handler requested.
      /// \return true if the handler was found.
      public: bool GetHandler(const std::string &_topic,
                              const std::string &_nUuid,
                              const std::string &_reqUuid,
                              std::shared_ptr<T> &_handler)
      {
        if (this->data.find(_topic) == this->data.end())
          return false;

        auto &m = this->data[_topic];
        if (m.find(_nUuid) == m.end())
          return false;

        if (m[_nUuid].find(_reqUuid) == m[_nUuid].end())
          return false;

        _handler = m[_nUuid][_reqUuid];
        return true;
      }

      /// \brief Add a request handler to a topic. A request handler stores
      /// the callback and types associated to a service call request.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \param[in] _handler Request handler.
      public: void AddHandler(const std::string &_topic,
                              const std::string &_nUuid,
                              const std::shared_ptr<T> &_handler)
      {
        // Create the topic entry.
        if (this->data.find(_topic) == this->data.end())
          this->data[_topic] = {};

        // Create the Node UUID entry.
        if (this->data[_topic].find(_nUuid) == this->data[_topic].end())
          this->data[_topic][_nUuid] = {};

        // Add/Replace the Req handler.
        this->data[_topic][_nUuid].insert(
          std::make_pair(_handler->GetHandlerUuid(), _handler));
      }

      /// \brief Return true if we have stored at least one request for the
      /// topic.
      /// \param[in] _topic Topic name.
      /// \return true if we have stored at least one request for the topic.
      public: bool HasHandlersForTopic(const std::string &_topic)
      {
        if (this->data.find(_topic) == this->data.end())
          return false;

        return !this->data[_topic].empty();
      }

      /// \brief Check if a node has at least one handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \return true if the node has at least one handler registered.
      public: bool HasHandlersForNode(const std::string &_topic,
                                      const std::string &_nUuid)
      {
        if (this->data.find(_topic) == this->data.end())
          return false;

        return this->data[_topic].find(_nUuid) != this->data[_topic].end();
      }

      /// \brief Remove a request handler. The node's uuid is used as a key to
      /// remove the appropriate request handler.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      /// \param[in] _reqUuid Request's UUID to remove.
      public: unsigned int RemoveHandler(const std::string &_topic,
                                         const std::string &_nUuid,
                                         const std::string &_reqUuid)
      {
        unsigned int counter = 0;
        if (this->data.find(_topic) != this->data.end())
        {
          if (this->data[_topic].find(_nUuid) != this->data[_topic].end())
          {
            this->data[_topic][_nUuid].erase(_reqUuid);
            if (this->data[_topic][_nUuid].empty())
              counter = this->data[_topic].erase(_nUuid);
            if (this->data[_topic].empty())
              this->data.erase(_topic);
          }
        }

        return counter;
      }

      /// \brief Remove all the handlers from a given node.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node's unique identifier.
      public: unsigned int RemoveHandlersForNode(const std::string &_topic,
                                         const std::string &_nUuid)
      {
        unsigned int counter = 0;
        if (this->data.find(_topic) != this->data.end())
        {
          counter = this->data[_topic].erase(_nUuid);
          if (this->data[_topic].empty())
            this->data.erase(_topic);
        }

        return counter;
      }

      /// \brief Stores all the service call data for each topic.
      private: std::map<std::string,
        std::map<std::string, std::map<std::string, std::shared_ptr<T>> >> data;
    };
  }
}

#endif
