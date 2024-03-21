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

#ifndef GZ_TRANSPORT_NODEPRIVATE_HH_
#define GZ_TRANSPORT_NODEPRIVATE_HH_

#include <memory>
#include <string>
#include <unordered_set>

#include "gz/transport/NetUtils.hh"
#include "gz/transport/NodeOptions.hh"
#include "gz/transport/Node.hh"
#include "gz/transport/NodeShared.hh"

namespace gz
{
  namespace transport
  {
    inline namespace GZ_TRANSPORT_VERSION_NAMESPACE
    {
    class NodeShared;

    /// \internal
    /// \brief Private data for Node class.
    class NodePrivate
    {
      /// \brief Constructor.
      public: NodePrivate() = default;

      /// \brief Destructor.
      public: virtual ~NodePrivate() = default;

      /// \brief Helper function for Subscribe.
      /// \param[in] _fullyQualifiedTopic Fully qualified topic name
      /// \return True on success.
      /// \sa TopicUtils::FullyQualifiedName
      public: bool SubscribeHelper(const std::string &_fullyQualifiedTopic);

      /// \brief Helper function to remove handlers from the shared publish
      /// queue. This is called when the node unsubscribes to a topic
      /// \param[in] _topic Topic that the node unsubcribed to.
      /// \return True on success.
      public: bool RemoveHandlersFromPubQueue(const std::string &_topic);

      /// \brief The list of topics subscribed by this node.
      public: std::unordered_set<std::string> topicsSubscribed;

      /// \brief The list of service calls advertised by this node.
      public: std::unordered_set<std::string> srvsAdvertised;

      /// \brief Node UUID. This ID is unique for each node.
      public: std::string nUuid;

      /// \brief Pointer to the object shared between all the nodes within the
      /// same process.
      public: std::shared_ptr<NodeShared> shared = NodeShared::SharedInstance();

      /// \brief Partition for this node.
      public: std::string partition = hostname() + ":" + username();

      /// \brief Default namespace for this node.
      public: std::string ns = "";

      /// \brief Custom options for this node.
      public: NodeOptions options;

      /// \brief Statistics publisher.
      public: Node::Publisher statPub;
    };
    }
  }
}
#endif
