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
#include <string>
#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class NodeOptionsPrivate;

    /// \class NodeOptions NodeOptions.hh ignition/transport/NodeOptions.hh
    /// \brief A class for customizing the behavior of the Node.
    /// E.g.: Set a custom namespace or a partition name.
    class IGNITION_VISIBLE NodeOptions
    {
      /// \brief Constructor.
      public: NodeOptions();

      /// \brief Copy constructor.
      /// \param[in] _other NodeOptions to copy.
      public: NodeOptions(const NodeOptions &_other);

      /// \brief Destructor.
      public: virtual ~NodeOptions();

      /// \brief Assignment operator.
      /// \param[in] _other The new NodeOptions.
      /// \return A reference to this instance.
      public: NodeOptions& operator=(const NodeOptions &_other);

      /// \brief Get the namespace used in this node.
      /// \return The namespace
      public: const std::string& NameSpace() const;

      /// \brief Set the node's namespace.
      /// \param[in] _ns The namespace.
      /// \return True when operation succeed or false if the namespace was
      /// invalid.
      public: bool SetNameSpace(const std::string &_ns);

      /// \brief Get the partition used in this node.
      /// \return The partition name.
      public: const std::string& Partition() const;

      /// \brief Set the node's partition name.
      /// \param[in] _ns The partition's name.
      /// \return True when operation succeed or false if the partition name was
      /// invalid.
      public: bool SetPartition(const std::string &_partition);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::NodeOptionsPrivate> dataPtr;
    };
  }
}
#endif
