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
      public: NodeOptions &operator=(const NodeOptions &_other);

      /// \brief Get the namespace used in this node.
      /// \return The namespace.
      /// \sa SetNameSpace.
      public: const std::string &NameSpace() const;

      /// \brief Set the node's namespace. A namespace is considered a prefix
      /// that might be potentially applied to some of the topic/services
      /// advertised in a node.
      /// E.g.: Node1 sets a namespace 'ns1' and advertises the topics
      /// 't1', 't2' and '/t3'. '/t3' is considered an absolute topic (starts
      /// with '/') and it won't be affected by a namespace. However, 't1' and
      /// 't2' will be advertised as '/ns1/t1' and '/ns1/t2'.
      /// A namespace is any alphanumeric string with a few exceptions.
      /// The symbol '/' is allowed as part of a namespace but just '/' is not
      /// allowed. The symbols '@', '~' and ' ' are not allowed as part of a
      /// namespace. Two or more consecutive slashes ('//') are not allowed.
      /// \param[in] _ns The namespace.
      /// \return True when operation succeed or false if the namespace was
      /// invalid.
      /// \sa NameSpace.
      public: bool SetNameSpace(const std::string &_ns);

      /// \brief Get the partition used in this node.
      /// \return The partition name.
      /// \sa SetPartition.
      public: const std::string &Partition() const;

      /// \brief Set the node's partition name. A partition is used to
      /// isolate a set of topics or services from other nodes that use the same
      /// names. E.g.: Node1 advertises topics '/foo' and Node2 advertises
      /// '/foo' too. If we don't use a partition, a node subscribed to '/foo'
      /// will receive the messages published from Node1 and Node2.
      /// Alternatively, we could specify 'p1' as a partition for Node1 and 'p2'
      /// as a partition for Node2. When we create the node for our subscriber,
      /// if we specify 'p1' as a partition name, we'll receive the messages
      /// published only by Node1. If we use 'p2', we'll only receive the
      /// messages published by Node2. If we don't set a partition name, we
      /// won't receive any messages from Node1 or Node2.
      /// A partition name is any alphanumeric string with a few exceptions.
      /// The symbol '/' is allowed as part of a partition name but just '/' is
      /// not allowed. The symbols '@', '~' and ' ' are not allowed as part of a
      /// partition name. Two or more consecutive slashes ('//') are not allowed
      /// \param[in] _ns The partition's name.
      /// The default partition value is created using a combination of your
      /// hostname, followed by ':' and your username. E.g.: "bb9:caguero" .
      /// It's also possible to use the environment variable IGN_PARTITION for
      /// setting a partition name.
      /// \return True when operation succeed or false if the partition name was
      /// invalid.
      /// \sa Partition
      public: bool SetPartition(const std::string &_partition);

      /// \internal
      /// \brief Smart pointer to private data.
      protected: std::unique_ptr<transport::NodeOptionsPrivate> dataPtr;
    };
  }
}
#endif
