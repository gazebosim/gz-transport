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

#include <google/protobuf/message.h>
#include <uuid/uuid.h>
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
        : hUuid(Uuid().ToString()),
          reqHash(0),
          repHash(0)
      {
      }

      /// \brief Destructor.
      public: virtual ~IRepHandler() = default;

      /// \brief Executes the local callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _msgReq Input parameter (Protobuf message).
      /// \param[out] _msgRep Output parameter (Protobuf message).
      /// \param[out] _result Service call result.
      public: virtual void RunLocalCallback(const std::string &_topic,
                                            const transport::ProtoMsg &_msgReq,
                                            transport::ProtoMsg &_msgRep,
                                            bool &_result) = 0;

      /// \brief Executes the callback registered for this handler.
      /// \param[in] _topic Topic to be passed to the callback.
      /// \param[in] _reqTypeName Protobuf name of the message request.
      /// \param[in] _reqHash Hash computed from the protobuf definition of the
      /// message request.
      /// \param[in] _req Serialized data received. The data will be used
      /// to compose a specific protobuf message and will be passed to the
      /// callback function.
      /// \param[in] _repTypeName Protobuf name of the message response.
      /// \param[in] _repHash Hash computed from the protobuf definition of the
      /// message response.
      /// \param[out] _rep Out parameter with the data serialized.
      /// \param[out] _result Service call result.
      public: virtual void RunCallback(const std::string &_topic,
                                       const std::string &_reqTypeName,
                                       const size_t _reqHash,
                                       const std::string &_req,
                                       const std::string &_repTypeName,
                                       const size_t _repHash,
                                       std::string &_rep,
                                       bool &_result) = 0;

      /// \brief Get the unique UUID of this handler.
      /// \return a string representation of the handler UUID.
      public: std::string GetHandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Unique handler's UUID.
      protected: std::string hUuid;

      /// \brief Name of the protobuf message used for the request. This name
      /// will be checked with the request type contained in every service
      /// call received to make sure that the type names match.
      protected: std::string reqTypeName;

      /// \brief Hash based on the protobuf message definition for the request.
      /// This hash will be compared with the hash received in the request
      /// parameter of every service call to guarantee that the protobuf message
      /// definition is the same between responser and requester.
      protected: size_t reqHash;

      /// \brief Name of the protobuf message used for the response. This name
      /// will be checked with the response type contained in every service
      /// call received to make sure that the type names match.
      protected: std::string repTypeName;

      /// \brief Hash based on the protobuf message definition for the response.
      /// This hash will be compared with the hash received in the response
      /// parameter of every service call to guarantee that the protobuf message
      /// definition is the same between responser and requester.
      protected: size_t repHash;
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
      public: RepHandler()
      {
        Req req;
        Rep rep;
        std::hash<std::string> hashFn;
        auto reqDescriptor = req.GetDescriptor();
        auto repDescriptor = rep.GetDescriptor();
        this->reqTypeName = reqDescriptor->name();
        this->reqHash = hashFn(reqDescriptor->DebugString());
        this->repTypeName = repDescriptor->name();
        this->repHash = hashFn(repDescriptor->DebugString());
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback with the following parameters:
      /// \param[in] _topic Service name.
      /// \param[in] _req Protobuf message containing the service request params
      /// \param[out] _rep Protobuf message containing the service response.
      /// \param[out] _result True when the service response is considered
      /// successful or false otherwise.
      public: void SetCallback(const std::function
        <void(const std::string &_topic, const Req &, Rep &, bool &)> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited.
      public: void RunLocalCallback(const std::string &_topic,
                                    const transport::ProtoMsg &_msgReq,
                                    transport::ProtoMsg &_msgRep,
                                    bool &_result)
      {
        // Execute the callback (if existing)
        if (this->cb)
        {
          auto msgReq = google::protobuf::down_cast<const Req*>(&_msgReq);
          auto msgRep = google::protobuf::down_cast<Rep*>(&_msgRep);
          this->cb(_topic, *msgReq, *msgRep, _result);
        }
        else
        {
          std::cerr << "RepHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          _result = false;
        }
      }

      // Documentation inherited.
      public: void RunCallback(const std::string &_topic,
                               const std::string &_reqTypeName,
                               const size_t _reqHash,
                               const std::string &_req,
                               const std::string &_repTypeName,
                               const size_t _repHash,
                               std::string &_rep,
                               bool &_result)
      {
        // The protobuf request names between responser and requester does not
        // match.
        if (_reqTypeName != this->reqTypeName)
        {
          std::cerr << "Reception error: Received a protobuf request ["
                    << _reqTypeName << "] but the expected type was ["
                    << this->reqTypeName << "]" << std::endl;
          _result = false;
          return;
        }

        // The protobuf response names between responser and requester does not
        // match.
        if (_repTypeName != this->repTypeName)
        {
          std::cerr << "Reception error: Received a protobuf response ["
                    << _repTypeName << "] but the expected type was ["
                    << this->repTypeName << "]" << std::endl;
          _result = false;
          return;
        }

        // The protobuf request definition between responser and requester does
        // not match.
        if (_reqHash != this->reqHash)
        {
          std::cerr << "Reception error: The local protobuf request definition "
                    << "does not match with the requester's definition. This "
                    << "might be caused by using different versions of the "
                    << "same message." << std::endl;
          _result = false;
          return;
        }

        // The protobuf response definition between responser and requester does
        // not match.
        if (_repHash != this->repHash)
        {
          std::cerr << "Reception error: The local protobuf response definition"
                    << " does not match with the requester's definition. This "
                    << "might be caused by using different versions of the "
                    << "same message." << std::endl;
          _result = false;
          return;
        }

        // Execute the callback (if existing).
        if (this->cb)
        {
          // Instantiate the specific protobuf message associated to this topic.
          Rep msgRep;

          auto msgReq = this->CreateMsg(_req.c_str());
          this->cb(_topic, *msgReq, msgRep, _result);
          msgRep.SerializeToString(&_rep);
        }
        else
        {
          std::cerr << "RepHandler::RunCallback() error: "
                    << "Callback is NULL" << std::endl;
          _result = false;
        }
      }

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \return Pointer to the specific protobuf message.
      private: std::shared_ptr<Req> CreateMsg(const char *_data)
      {
        // Instantiate a specific protobuf message
        std::shared_ptr<Req> msgPtr(new Req());

        // Create the message using some serialized data
        msgPtr->ParseFromString(_data);

        return msgPtr;
      }

      /// \brief Callback to the function registered for this handler.
      private: std::function
        <void(const std::string &, const Req &, Rep &, bool &)> cb;
    };
  }
}

#endif
