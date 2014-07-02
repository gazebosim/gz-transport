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

#ifndef __IGN_TRANSPORT_ADVERTISEHANDLER_HH_INCLUDED__
#define __IGN_TRANSPORT_ADVERTISEHANDLER_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

namespace ignition
{
  namespace transport
  {
    /// \class IAdvertiseHandler AdvertiseHandler.hh
    /// ignition/transport/AdvertiseHandler.hh
    /// \brief Interface class used to check that the type published in a
    /// message is the same as the type announced during the Advertise().
    class IGNITION_VISIBLE IAdvertiseHandler
    {
      /// \brief Constructor.
      /// \param[in] _nUuid UUID of the node registering the handler.
      public: IAdvertiseHandler(const std::string &_nUuid)
        : hUuid(Uuid().ToString()),
          nUuid(_nUuid)
      {
      }

      /// \brief Destructor.
      public: virtual ~IAdvertiseHandler()
      {
      }

      public: virtual bool CheckMsg (const transport::ProtoMsg &_msg) = 0;


      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      public: std::string GetNodeUuid()
      {
        return this->nUuid;
      }

      /// \brief Get the unique UUID of this handler.
      /// \return a string representation of the handler UUID.
      public: std::string GetHandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Unique handler's UUID.
      private: std::string hUuid;

      /// \brief Node UUID.
      private: std::string nUuid;
    };

    /// \class AdvertiseHandler AdvertiseHandler.hh
    /// ignition/transport/AdvertiseHandler.hh
    /// \brief .
    template <typename T> class AdvertiseHandler
      : public IAdvertiseHandler
    {
      // Documentation inherited.
      public: AdvertiseHandler(const std::string &_nUuid)
        : IAdvertiseHandler(_nUuid)
      {
      }

      // Documentation inherited.
      public: bool CheckMsg(const transport::ProtoMsg &_msg)
      {
        google::protobuf::down_cast<const T*>(&_msg);
        return true;
      }

      private: T advertisedType;
    };
  }
}

#endif
