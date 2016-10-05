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

#ifndef __IGN_TRANSPORT_REPHANDLER_HH_INCLUDED__
#define __IGN_TRANSPORT_REPHANDLER_HH_INCLUDED__

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if GOOGLE_PROTOBUF_VERSION > 2999999
#include <google/protobuf/stubs/casts.h>
#endif

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
    /// \class IRepHandler RepHandler.hh ignition/transport/RepHandler.hh
    /// \brief Interface class used to manage a replier handler.
    class IGNITION_VISIBLE IRepHandler
    {
      /// \brief Constructor.
      public: IRepHandler()
        : hUuid(Uuid().ToString())
      {
      }

      /// \brief Destructor.
      public: virtual ~IRepHandler() = default;

      /// \brief Executes the local callback registered for this handler.
      /// \param[in] _msgReq Input parameter (Protobuf message).
      /// \param[out] _msgRep Output parameter (Protobuf message).
      /// \param[out] _result Service call result.
      public: virtual void RunLocalCallback(const transport::ProtoMsg &_msgReq,
                                            transport::ProtoMsg &_msgRep,
                                            bool &_result) = 0;

      /// \brief Executes the callback registered for this handler.
      /// \param[in] _req Serialized data received. The data will be used
      /// to compose a specific protobuf message and will be passed to the
      /// callback function.
      /// \param[out] _rep Out parameter with the data serialized.
      /// \param[out] _result Service call result.
      public: virtual void RunCallback(const std::string &_req,
                                       std::string &_rep,
                                       bool &_result) = 0;

      /// \brief Get the unique UUID of this handler.
      /// \return a string representation of the handler UUID.
      public: std::string HandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Get the message type name used in the service request.
      /// \return Message type name.
      public: virtual std::string ReqTypeName() const = 0;

      /// \brief Get the message type name used in the service response.
      /// \return Message type name.
      public: virtual std::string RepTypeName() const = 0;

      /// \brief Unique handler's UUID.
      protected: std::string hUuid;
    };

    /// \class RepHandler RepHandler.hh
    /// \brief It creates a service reply handler for a pair of protobuf
    /// messages containing the request parameters and the response.
    /// 'Req' is the protobuf message type containing the input parameters of
    // the service call. 'Rep' is the protobuf message type that will be filled
    /// with the service response.
    template <typename Req, typename Rep> class RepHandler
      : public IRepHandler
    {
      // Documentation inherited.
      public: RepHandler() = default;

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback with the following parameters:
      /// \param[in] _req Protobuf message containing the service request params
      /// \param[out] _rep Protobuf message containing the service response.
      /// \param[out] _result True when the service response is considered
      /// successful or false otherwise.
      public: void SetCallback(const std::function
        <void(const Req &, Rep &, bool &)> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited.
      public: void RunLocalCallback(const transport::ProtoMsg &_msgReq,
                                    transport::ProtoMsg &_msgRep,
                                    bool &_result)
      {
        // Execute the callback (if existing)
        if (this->cb)
        {
          #if GOOGLE_PROTOBUF_VERSION > 2999999
          auto msgReq = google::protobuf::down_cast<const Req*>(&_msgReq);
          auto msgRep = google::protobuf::down_cast<Rep*>(&_msgRep);
          #else
          auto msgReq =
            google::protobuf::internal::down_cast<const Req*>(&_msgReq);
          auto msgRep = google::protobuf::internal::down_cast<Rep*>(&_msgRep);
          #endif

          this->cb(*msgReq, *msgRep, _result);
        }
        else
        {
          std::cerr << "RepHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          _result = false;
        }
      }

      // Documentation inherited.
      public: void RunCallback(const std::string &_req,
                               std::string &_rep,
                               bool &_result)
      {
        // Check if we have a callback registered.
        if (!this->cb)
        {
          std::cerr << "RepHandler::RunCallback() error: "
                    << "Callback is NULL" << std::endl;
          _result = false;
          return;
        }

        // Instantiate the specific protobuf message associated to this topic.
        auto msgReq = this->CreateMsg(_req);
        if (!msgReq)
        {
          _result = false;
          return;
        }

        Rep msgRep;
        this->cb(*msgReq, msgRep, _result);

        if (!msgRep.SerializeToString(&_rep))
        {
          std::cerr << "RepHandler::RunCallback(): Error serializing the "
                    << "response" << std::endl;
          _result = false;
          return;
        }
      }

      // Documentation inherited.
      public: virtual std::string ReqTypeName() const
      {
        return Req().GetTypeName();
      }

      // Documentation inherited.
      public: virtual std::string RepTypeName() const
      {
        return Rep().GetTypeName();
      }

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \return Pointer to the specific protobuf message.
      private: std::shared_ptr<Req> CreateMsg(const std::string &_data) const
      {
        // Instantiate a specific protobuf message
        std::shared_ptr<Req> msgPtr(new Req());

        // Create the message using some serialized data
        if (!msgPtr->ParseFromString(_data))
        {
          std::cerr << "RepHandler::CreateMsg() error: ParseFromString failed"
                    << std::endl;
        }

        return msgPtr;
      }

      /// \brief Callback to the function registered for this handler.
      private: std::function<void(const Req &, Rep &, bool &)> cb;
    };
  }
}

#endif
