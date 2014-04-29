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

#ifndef __IGN_TRANSPORT_MSGSUBSCRIBER_HH_INCLUDED__
#define __IGN_TRANSPORT_MSGSUBSCRIBER_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <memory>

namespace ignition
{
  namespace transport
  {
    //
    class ISubscriptionHandler
    {
      public: virtual void RunCallback(const std::string &,
                                       const std::string &) = 0;
    };

    //
    template <typename T> class SubscriptionHandler
      : public ISubscriptionHandler
    {
      public: std::shared_ptr<T>
        CreateMsg(const char *_data)
      {
        std::shared_ptr<T> msgPtr(new T());
        msgPtr->ParseFromString(_data);
        return msgPtr;
      }

      public: void SetCallback(
        const std::function
          <void (const std::string &, const std::shared_ptr<T> &)> &_cb)
      {
        this->cb = _cb;
      }

      public: void RunCallback(const std::string &_topic,
                               const std::string &_data)
      {
        std::shared_ptr<T> msg;
        msg = this->CreateMsg(_data.c_str());

        if (this->cb)
          this->cb(_topic, msg);
        else
          std::cerr << "Callback is NULL" << std::endl;
      }

      private: std::function
          <void (const std::string &, const std::shared_ptr<T> &)> cb;
    };
  }
}

#endif
