/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#ifndef IGNITION_TRANSPORT_LOG_MSGITER_HH_
#define IGNITION_TRANSPORT_LOG_MSGITER_HH_

#include <memory>

#include <ignition/transport/log/Export.hh>
#include <ignition/transport/log/Message.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      /// \brief Forward Declarations
      class MsgIterPrivate;
      class Log;

      /// \brief Implements iterator for reading messages
      class IGNITION_TRANSPORT_LOG_VISIBLE MsgIter
      {
        /// \brief Default constructor
        public: MsgIter();

        /// \brief Copy Constructor
        /// \param[in] _orig the instance being copied
        // TODO
        // public: MsgIter(const MsgIter &_orig);

        /// \brief Move Constructor
        /// \param[in] _orig the instance being copied
        public: MsgIter(MsgIter &&_orig);

        /// \brief Destructor;
        public: ~MsgIter();

        /// \brief Copy-assignment operator
        /// \param[in] _orig the instance being copied
        /// \return a reference to this instance
        // public: MsgIter &operator=(const MsgIter &_orig);

        /// \brief Prefix increment
        /// \return a reference to this instance
        public: MsgIter &operator++();

        /// \brief Postfix increment
        /// \return a reference to a copy of this iterator before incrementing
        // TODO
        // public: MsgIter operator++(int);

        /// \brief Equality operator
        /// \param[in] _other the iterator this is being compared to
        /// \return true if the two iterator point to the same message
        public: bool operator==(const MsgIter &_other) const;

        /// \brief Inequality operator
        /// \param[in] _other the iterator this is being compared to
        /// \return false if the two iterator point to the same message
        public: bool operator!=(const MsgIter &_other) const;

        /// \brief Dereference Operator
        /// \return a reference to the message this is pointing to
        public: const Message &operator*() const;

        /// \brief arrow dereference operator
        /// \return a pointer to the message this is pointing to
        public: const Message *operator->() const;

        /// \brief Private Implementation Pointer
        public: std::unique_ptr<MsgIterPrivate> dataPtr;

        /// \brief Construct with private implementation 
        /// \param[in] _pimpl a private implementation pointer
        /// \internal
        private: MsgIter(std::unique_ptr<MsgIterPrivate> &&_pimpl);

        /// \brief can use private constructor
        friend class Batch;
      };
    }
  }
}
#endif
