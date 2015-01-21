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

#include "ignition/transport/SubscriberOptions.hh"
#include "ignition/transport/SubscriberOptionsPrivate.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
SubscriberOptions::SubscriberOptions()
  : dataPtr(new SubscriberOptionsPrivate())
{
}

//////////////////////////////////////////////////
void SubscriberOptions::Debug(bool _newValue)
{
  this->dataPtr->debug = _newValue;
}

//////////////////////////////////////////////////
bool SubscriberOptions::Debug()
{
  return this->dataPtr->debug;
}
