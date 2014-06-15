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

#include <uuid/uuid.h>
#include <string>
#include <vector>
#include "ignition/transport/Uuid.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Uuid::Uuid()
{
  uuid_generate(this->data);
}

//////////////////////////////////////////////////
Uuid::~Uuid()
{
  uuid_clear(this->data);
}

//////////////////////////////////////////////////
std::string Uuid::ToString() const
{
  std::vector<char> uuidStr(Uuid::UuidStrLen);

  for (size_t i = 0; i < sizeof(uuid_t) && i != Uuid::UuidStrLen; ++i)
  {
    snprintf(&uuidStr[0], Uuid::UuidStrLen,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      this->data[0], this->data[1], this->data[2], this->data[3],
      this->data[4], this->data[5], this->data[6], this->data[7],
      this->data[8], this->data[9], this->data[10], this->data[11],
      this->data[12], this->data[13], this->data[14], this->data[15]);
  }
  return std::string(uuidStr.begin(), uuidStr.end() - 1);
}
