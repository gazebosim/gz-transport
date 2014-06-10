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

#ifndef __IGN_TRANSPORT_NODEPRIVATE_HH_INCLUDED__
#define __IGN_TRANSPORT_NODEPRIVATE_HH_INCLUDED__

#include <uuid/uuid.h>
#include <string>
#include <unordered_set>
#include <vector>
#include "ignition/transport/NodeShared.hh"

using namespace ignition;
using namespace transport;

namespace ignition
{
  namespace transport
  {
    /// \class NodePrivate NodePrivate.hh
    /// \brief Private data fPrivate Node class.
    class NodePrivate
    {
      /// \brief Constructor.
      public: NodePrivate() = default;

      /// \brief Destructor.
      public: ~NodePrivate() = default;

      /// \brief The list of topics subscribed by this node.
      public: std::vector<std::string> topicsSubscribed;

      /// \brief The list of topics advertised by this node.
      public: std::unordered_set<std::string> topicsAdvertised;

      /// \brief The list of service calls advertised by this node.
      public: std::vector<std::string> srvsAdvertised;

      /// \brief Node UUID. This ID is unique for each node.
      public: uuid_t nUuid;

      /// \brief Node UUID in string format.
      public: std::string nUuidStr;

      /// \brief Pointer to the object shared between all the nodes within the
      /// same process.
      public: NodeSharedPtr shared;

      /// \brief Print activity to stdout.
      public: bool verbose;
    };
  }
}
#endif
