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
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class IRepHandler RepHandler.hh
    /// \brief Interface class used to manage generic protobub messages.
    class IRepHandler
    {
      /// \brief Constructor.
      /// \param[in] _uuid UUID of the node registering the response handler.
      public: IRepHandler(const std::string &_nUuid)
        : nUuidStr(_nUuid)
      {
      }

      /// \brief Destructor.
      public: virtual ~IRepHandler()
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
      /// \param[in] _req Serialized data received. The data will be used
      /// to compose a specific protobuf message and will be passed to the
      /// callback function.
      /// \return 0 when success.
      public: virtual int RunCallback(const std::string &_topic,
                                      const std::string &_req,
                                      std::string &_rep,
                                      bool &_result) = 0;

      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      /*public: std::string GetNodeUuid()
      {
        return this->nUuidStr;
      }*/

      /// \brief Node UUID (string).
      private: std::string nUuidStr;
    };

    /// \class RepHandler RepHandler.hh
    /// \brief It creates service reply handlers for each specific protobuf
    /// message used.
    template <typename T1, typename T2> class RepHandler
      : public IRepHandler
    {
      // Documentation inherited.
      public: RepHandler(const std::string &_nUuid)
        : IRepHandler(_nUuid)
      {
      }

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \return Pointer to the specific protobuf message.
      public: std::shared_ptr<T1> CreateMsg(const char *_data)
      {
        // Instantiate a specific protobuf message
        std::shared_ptr<T1> msgPtr(new T1());

        // Create the message using some serialized data
        msgPtr->ParseFromString(_data);

        return msgPtr;
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback.
      public: void SetCallback(
        const std::function<bool(const std::string &, const T1 &, T2 &)> &_cb)
      {
        this->cb = _cb;
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
          std::cerr << "RepHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          return -1;
        }
      }*/

      // Documentation inherited.
      public: int RunCallback(const std::string &_topic,
                              const std::string &_req,
                              std::string &_rep,
                              bool &_result)
      {
        // Execute the callback (if existing).
        if (this->cb)
        {
          // Instantiate the specific protobuf message associated to this topic.
          T2 msgRep;

          auto msgReq = this->CreateMsg(_req.c_str());
          _result = this->cb(_topic, *msgReq, msgRep);
          msgRep.SerializeToString(&_rep);

          return 0;
        }
        else
        {
          std::cerr << "RepHandler::RunCallback() error: "
                    << "Callback is NULL" << std::endl;
          _result = false;
          return -1;
        }
      }

      /// \brief Callback to the function registered for this handler.
      private: std::function<bool(const std::string &, const T1 &, T2 &)> cb;
    };
  }
}

#endif
