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

#ifndef __IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_INCLUDED__
#define __IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <functional>
#include <memory>
#include <string>

namespace ignition
{
  namespace transport
  {
    /// \brief Interface class used when the specific protobuf message
    /// is not known.
    class ISubscriptionHandler
    {
      /// \brief Executes the callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _data Serialized data received. The data will be used
      /// to compose a specific protobuf message and will be passed to the
      /// callback function.
      /// \return 0 when success.
      public: virtual int RunCallback(const std::string &_topic,
                                      const std::string &_data) = 0;
    };

    /// \brief generic Subscription handler class. It creates subscription
    /// handlers for each specific protobuf messages used.
    template <typename T> class SubscriptionHandler
      : public ISubscriptionHandler
    {
      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      public: std::shared_ptr<T> CreateMsg(const char *_data)
      {
        // Instantiate a specific protobuf message
        std::shared_ptr<T> msgPtr(new T());

        // Create the message using some serialized data
        msgPtr->ParseFromString(_data);

        return msgPtr;
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback.
      public: void SetCallback(
        const std::function
          <void (const std::string &, const std::shared_ptr<T> &)> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited
      public: int RunCallback(const std::string &_topic,
                              const std::string &_data)
      {
        // Instantiate the specific protobuf message associated to this topic.
        std::shared_ptr<T> msg = this->CreateMsg(_data.c_str());

        // Execute the callback (if existing)
        if (this->cb)
        {
          this->cb(_topic, msg);
          return 0;
        }
        else
        {
          std::cerr << "Callback is NULL" << std::endl;
          return -1;
        }
      }

      /// \brief Callback to the function registered for this handler.
      private: std::function
          <void (const std::string &, const std::shared_ptr<T> &)> cb;
    };
  }
}

#endif
