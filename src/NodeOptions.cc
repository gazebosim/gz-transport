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

#include <iostream>
#include <string>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/NodeOptionsPrivate.hh"
#include "ignition/transport/TopicUtils.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
NodeOptions::NodeOptions()
  : dataPtr(new NodeOptionsPrivate())
{
  // Check if the environment variable IGN_PARTITION is present.
  std::string ignPartition;
  if (env("IGN_PARTITION", ignPartition))
    this->SetPartition(ignPartition);
}

//////////////////////////////////////////////////
NodeOptions::NodeOptions(const NodeOptions &_other)
  : dataPtr(new NodeOptionsPrivate())
{
  (*this) = _other;
}

//////////////////////////////////////////////////
NodeOptions::~NodeOptions()
{
}

//////////////////////////////////////////////////
NodeOptions &NodeOptions::operator=(const NodeOptions &_other)
{
  this->SetNameSpace(_other.NameSpace());
  this->SetPartition(_other.Partition());
  return *this;
}

//////////////////////////////////////////////////
const std::string &NodeOptions::NameSpace() const
{
  return this->dataPtr->ns;
}

//////////////////////////////////////////////////
bool NodeOptions::SetNameSpace(const std::string &_ns)
{
  if (!TopicUtils::IsValidNamespace(_ns))
  {
    std::cerr << "Invalid namespace [" << _ns << "]" << std::endl;
    return false;
  }
  this->dataPtr->ns = _ns;
  return true;
}

//////////////////////////////////////////////////
const std::string &NodeOptions::Partition() const
{
  return this->dataPtr->partition;
}

//////////////////////////////////////////////////
bool NodeOptions::SetPartition(const std::string &_partition)
{
  if (!TopicUtils::IsValidPartition(_partition))
  {
    std::cerr << "Invalid partition name [" << _partition << "]" << std::endl;
    return false;
  }
  this->dataPtr->partition = _partition;
  return true;
}
