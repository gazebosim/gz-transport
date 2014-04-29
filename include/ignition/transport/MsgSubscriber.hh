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

namespace ignition
{
  namespace transport
  {
    // ToDo
    template <typename T> class IMsgSubscriber
    {
      /// \brief Constructor.
      public: IMsgSubscriber(){};

      /// \brief Destructor.
      public: virtual ~IMsgSubscriber(){};

      public: virtual void CreateMsg(const char *_data, T &_msg) = 0;
    };

    template <typename T> class MsgSubscriber : public IMsgSubscriber<T>
    {
      /// \brief Constructor.
      public: MsgSubscriber(){};

      /// \brief Destructor.
      public: virtual ~MsgSubscriber(){};

      public: void CreateMsg(const char *_data, T &_msg)
      {
        _msg.ParseFromString(_data);
      }
    };
  }
}

#endif
