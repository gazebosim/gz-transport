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

#ifndef __IGN_TRANSPORT_NODE_HH_INCLUDED__
#define __IGN_TRANSPORT_NODE_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <string>
#include "ignition/transport/MsgSubscriber.hh"
#include "ignition/transport/NodePrivate.hh"

namespace ignition
{
  namespace transport
  {
    class Node
    {
      /// \brief Constructor.
      /// \param[in] _verbose true for enabling verbose mode.
      public: Node(bool _verbose);

      /// \brief Destructor.
      public: virtual ~Node();

      /// \brief Advertise a new service.
      /// \param[in] _topic Topic to be advertised.
      /// \return 0 when success.
      public: int Advertise(const std::string &_topic);

      /// \brief Unadvertise a new service.
      /// \param[in] _topic Topic to be unadvertised.
      /// \return 0 when success.
      public: int UnAdvertise(const std::string &_topic);

      /// \ Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _message protobuf message.
      /// \return 0 when success.
      public: int Publish(const std::string &_topic,
                     const std::shared_ptr<google::protobuf::Message> &_msgPtr);

      /// \brief Subscribe to a topic registering a callback.
      /// \param[in] _topic Topic to be subscribed.
      /// \param[in] _cb Pointer to the callback function.
      /// \return 0 when success.
      public: int Subscribe(const std::string &_topic,
                            const transport::TopicInfo::Callback &_cb);


      public: int SubscribeLocal(const std::string &_topic,
                                const transport::TopicInfo::CallbackLocal &_cb);

      public:
      template<class T>
      int SubscribeT(const std::string &_topic,
          void(*_cb)(const std::string &_topic,
                     const std::shared_ptr<T> &_msg))
      {
        MsgSubscriber<T> msgSubscriber;
        return 0;
      }

      /// \brief Subscribe to a topic registering a callback.
      /// \param[in] _topic Topic to be unsubscribed.
      /// \return 0 when success.
      public: int UnSubscribe(const std::string &_topic);

      /// \internal
      /// \brief Pointer to private data.
      protected: transport::NodePrivate &dataPtr;
    };
  }
}
#endif
