/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#ifndef IGNITION_TRANSPORT_LOG_BATCH_HH_
#define IGNITION_TRANSPORT_LOG_BATCH_HH_

#include <ios>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <ignition/common/Time.hh>
#include <ignition/transport/log/Export.hh>
#include <ignition/transport/log/MsgIter.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      /// \brief Forward declaration
      class BatchPrivate;
      class Log;

      /// \brief Holds the result of a query for messages
      class IGNITION_TRANSPORT_LOG_VISIBLE Batch
      {
        /// \brief Default constructor
        public: Batch();

        /// \brief move constructor
        /// \param[in] _old the instance being moved into this one
        public: Batch(Batch &&_old);  // NOLINT

        /// \brief destructor
        public: ~Batch();

        /// \brief typedef for prettiness
        public: using iterator = MsgIter;

        /// \brief Iterator to first message in batch
        /// \remarks the lowercase function name is required to support
        ///   range-based for loops
        public: iterator begin();

        /// \brief Iterator to one past the last message in a batch
        /// \remarks the lowercase function name is required to support
        ///   range-based for loops
        public: iterator end();

        /// \brief Private implementation
        private: std::unique_ptr<BatchPrivate> dataPtr;

        /// \brief Construct with private implementation
        /// \param[in] _pimpl a private implementation pointer
        /// \internal
        private: Batch(
            std::unique_ptr<BatchPrivate> &&_pimpl);  // NOLINT(build/c++11)

        /// \brief Log can use private constructor
        friend class Log;
      };
    }
  }
}
#endif
