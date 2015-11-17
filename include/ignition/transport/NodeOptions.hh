/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifndef __IGN_TRANSPORT_NODEOPTIONS_HH_INCLUDED__
#define __IGN_TRANSPORT_NODEOPTIONS_HH_INCLUDED__

#include <memory>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class NodeOptionsPrivate;

    /// \class NodeOptions NodeOptions.hh ignition/transport/NodeOptions.hh
    /// \brief A class for customizing the behavior of the Node.
    /// E.g.: Specify the size of the queues, maximum frequency of publication.
    class IGNITION_VISIBLE NodeOptions
    {
      /// \brief Constructor.
      public: NodeOptions();

      /// \brief Destructor.
      public: virtual ~NodeOptions();

      /// \brief Copy constructor.
      public: NodeOptions(const NodeOptions &_other);

      /// \brief Set the maximum rate of message or service publication.
      /// \param[in] _hzRate Maximum rate of publication (Hz).
      public: void SetMaxRate(const unsigned int _hzRate);

      /// \brief Get the maximum rate of message or service publication.
      /// \return Maximum rate of publication (Hz).
      public: unsigned int MaxRate() const;

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::NodeOptionsPrivate> dataPtr;
    };
  }
}
#endif
