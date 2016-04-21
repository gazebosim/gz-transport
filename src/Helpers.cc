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

#include <cstdlib>
#include <string>

#include "ignition/transport/Helpers.hh"

//////////////////////////////////////////////////
namespace ignition
{
  namespace transport
  {
    bool env(const std::string &_name, std::string &_value)
    {
      char *v;
#ifdef _MSC_VER
      size_t sz = 0;
      _dupenv_s(&v, &sz, _name.c_str());
#else
      v = std::getenv(_name.c_str());
#endif
      if (v)
      {
        _value = v;
        return true;
      }
      return false;
    }
  }
}
