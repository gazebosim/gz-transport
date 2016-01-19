/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#include "ignition/transport/SubscribeOptions.hh"

using namespace ignition;
using namespace transport;

struct SubscribeOptionsPrivate
{
  /// \brief Enable the text conversion for messages.
  bool textConversionEnabled = false;
};

//////////////////////////////////////////////////
SubscribeOptions::SubscribeOptions()
  : dataPtr(new SubscribeOptionsPrivate())
{
}

//////////////////////////////////////////////////
SubscribeOptions::SubscribeOptions(const SubscribeOptions &_other)
  : dataPtr(new SubscribeOptionsPrivate())
{
  (*this) = _other;
}

//////////////////////////////////////////////////
SubscribeOptions::~SubscribeOptions()
{
}

//////////////////////////////////////////////////
SubscribeOptions &SubscribeOptions::operator=(const SubscribeOptions &_other)
{
  this->SetScope(_other.Scope());
  return *this;
}

//////////////////////////////////////////////////
bool SubscribeOptions::TextConversion() const
{
  return this->dataPtr->textConversionEnabled;
}

//////////////////////////////////////////////////
void SubscribeOptions::SetTextConversion(const bool _enabled);
{
  this->dataPtr->textConversionEnabled = _enabled;
}
