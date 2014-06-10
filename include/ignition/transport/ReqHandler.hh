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

#ifndef __IGN_TRANSPORT_REQHANDLER_HH_INCLUDED__
#define __IGN_TRANSPORT_REQHANDLER_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <uuid/uuid.h>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class IReqHandler ReqHandler.hh
    /// \brief Interface class used to manage a request handler.
    class IReqHandler
    {
      /// \brief Constructor.
      /// \param[in] _uuid UUID of the node registering the request handler.
      public: IReqHandler(const std::string &_nUuid)
        : nUuidStr(_nUuid),
          requested(false),
          repAvailable(false),
          result(false)
      {
        uuid_t uuid;
        uuid_generate(uuid);
        this->reqUuid = GetGuidStr(uuid);
      }

      /// \brief Destructor.
      public: virtual ~IReqHandler()
      {
      }

      /// \brief Executes the local callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _msg Protobuf message received.
      /// \return 0 when success.
     /* public: virtual int RunLocalCallback(const std::string &_topic,
                                           const transport::ProtoMsg &_msg) = 0;
      */
      /// \brief Executes the callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _data Serialized data received. The data will be used
      /// to compose a specific protobuf message and will be passed to the
      /// callback function.
      /// \return 0 when success.
      public: virtual void NotifyResult(const std::string &_topic,
                                        const std::string &_rep,
                                        const bool _result) = 0;

      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      public: std::string GetNodeUuid()
      {
        return this->nUuidStr;
      }

      /// \brief
      public: bool Requested() const
      {
        return this->requested;
      }

      /// \brief
      public: void SetRequested(bool _value)
      {
        this->requested = _value;
      }

      /// \brief
      public: virtual std::string Serialize() = 0;

      /// \brief
      public: std::string GetReqUuid() const
      {
        return this->nUuidStr;
      }

      /// \brief Request's UUID.
      protected: std::string reqUuid;

      /// \brief Node UUID (string).
      private: std::string nUuidStr;

      /// \brief When true, the REQ was already sent and the REP should be on
      /// its way. Used to not resend the same REQ more than one time.
      private: bool requested;

      /// \brief
      public: bool repAvailable;

      /// \brief
      public: std::condition_variable_any condition;

      /// \brief
      public: std::string rep;

      /// \brief
      public: bool result;
    };

    /// \class ReqHandler ReqHandler.hh
    /// \brief It creates service reply handlers for each specific protobuf
    /// message used.
    template <typename T1, typename T2> class ReqHandler
      : public IReqHandler
    {
      // Documentation inherited.
      public: ReqHandler(const std::string &_nUuid)
        : IReqHandler(_nUuid)
      {
      }

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \return Pointer to the specific protobuf message.
      public: std::shared_ptr<T2> CreateMsg(const char *_data)
      {
        // Instantiate a specific protobuf message
        std::shared_ptr<T2> msgPtr(new T2());

        // Create the message using some serialized data
        msgPtr->ParseFromString(_data);

        return msgPtr;
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback.
      public: void SetCallback(
        const std::function<void(const std::string &, const T2 &, bool)> &_cb)
      {
        this->cb = _cb;
      }

      public: void SetMessage(const T1 &_reqMsg)
      {
        this->reqMsg = _reqMsg;
      }

      public: std::string Serialize()
      {
        std::string buffer;
        this->reqMsg.SerializeToString(&buffer);
        return buffer;
      }

      // Documentation inherited.
      /*public: int RunLocalCallback(const std::string &_topic,
                                   const transport::ProtoMsg &_msg)
      {
        // Execute the callback (if existing)
        if (this->cb)
        {
          auto msgPtr = google::protobuf::down_cast<const T*>(&_msg);
          this->cb(_topic, *msgPtr);
          return 0;
        }
        else
        {
          std::cerr << "ReqHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          return -1;
        }
      }*/

      // Documentation inherited.
      public: void NotifyResult(const std::string &_topic,
                                const std::string &_rep,
                                const bool _result)
      {
        // Execute the callback (if existing).
        if (this->cb)
        {
          // Instantiate the specific protobuf message associated to this topic.
          auto msg = this->CreateMsg(_rep.c_str());
          this->cb(_topic, *msg, _result);
        }
        else
        {
          this->rep = _rep;
          this->result = _result;
        }

        this->repAvailable = true;
        this->condition.notify_one();
      }

      // Protobuf message containing the request's parameters.
      private: T1 reqMsg;

      /// \brief Callback to the function registered for this handler.
      private: std::function<void(const std::string &, const T2 &, bool)> cb;
    };
  }
}

#endif
