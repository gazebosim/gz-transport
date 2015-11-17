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

#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/NodeOptionsPrivate.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
NodeOptions::NodeOptions()
  : dataPtr(new NodeOptionsPrivate())
{
}

//////////////////////////////////////////////////
NodeOptions::~NodeOptions()
{
}

//////////////////////////////////////////////////
NodeOptions::NodeOptions(const NodeOptions &_other)
{
  this->SetMaxRate(_other.MaxRate());
}

//////////////////////////////////////////////////
void NodeOptions::SetMaxRate(const unsigned int _hzRate)
{
  this->dataPtr->hzRate = _hzRate;
}

//////////////////////////////////////////////////
unsigned int NodeOptions::MaxRate() const
{
  return this->dataPtr->hzRate;
}
