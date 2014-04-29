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

namespace ignition
{
  namespace transport
  {
    //
    class ITest
    {
      public: virtual void test() = 0;
    };

    //
    class IMsgSubscriber
    {
      protected: IMsgSubscriber() {}
      public: virtual void CreateMsg(const char *_data,
                                     std::shared_ptr<google::protobuf::Message> &_msg) {}
    };

    //
    template <typename T> class MsgSubscriber : public IMsgSubscriber
    {
      public: MsgSubscriber() {}

      public: void CreateMsg(const char *_data, std::shared_ptr<T> &_msg)
      {
        _msg.ParseFromString(_data);
      }
    };
  }
}

#endif
