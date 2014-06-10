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
        : result(false),
          nUuidStr(_nUuid),
          requested(false),
          repAvailable(false)
      {
        uuid_t uuid;
        uuid_generate(uuid);
        this->hUuid = GetGuidStr(uuid);
      }

      /// \brief Destructor.
      public: virtual ~IReqHandler() = default;

      /// \brief Executes the callback registered for this handler and notify
      /// a potential requester waiting on a blocking call.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _rep Serialized data containing the response coming from
      /// the service call responser.
      /// \param[in] _result Contains the result of the service call coming from
      /// the service call responser.
      public: virtual void NotifyResult(const std::string &_topic,
                                        const std::string &_rep,
                                        const bool _result) = 0;

      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      public: std::string GetNodeUuid()
      {
        return this->nUuidStr;
      }

      /// \brief Returns if this service call request has already been requested
      /// \return True when the service call has been requested.
      public: bool Requested() const
      {
        return this->requested;
      }

      /// \brief Mark the service call as requested (or not).
      /// \param[in] _value true when you want to flag this REQ as requested.
      public: void SetRequested(bool _value)
      {
        this->requested = _value;
      }

      /// \brief Serialize the Req protobuf message stored.
      /// \return The serialized data.
      public: virtual std::string Serialize() = 0;

      /// \brief Returns the unique handler UUID.
      /// \returns The handler's UUID.
      public: std::string GetHandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Condition variable used to wait until a service call REP is
      /// available.
      public: std::condition_variable_any condition;

      /// \brief Stores the service call response as raw bytes.
      public: std::string rep;

      /// \brief Stores the result of the service call.
      public: bool result;

      /// \brief Unique handler's UUID.
      protected: std::string hUuid;

      /// \brief Node UUID (string).
      private: std::string nUuidStr;

      /// \brief When true, the REQ was already sent and the REP should be on
      /// its way. Used to not resend the same REQ more than one time.
      private: bool requested;

      /// \brief When there is a blocking service call request, the call can
      /// be unlocked when a service call REP is available. This variable
      /// captures if we have found a node that can satisty our request.
      public: bool repAvailable;
    };

    /// \class ReqHandler ReqHandler.hh
    /// \brief It creates a reply handler for the specific protobuf
    /// messages used.
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

      /// \brief Set the REQ protobuf message for this handler.
      /// \param[in] _reqMsg Input parameter of the service call (protobuf).
      public: void SetMessage(const T1 &_reqMsg)
      {
        this->reqMsg = _reqMsg;
      }

      // Documentation inherited
      public: std::string Serialize()
      {
        std::string buffer;
        this->reqMsg.SerializeToString(&buffer);
        return buffer;
      }

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
