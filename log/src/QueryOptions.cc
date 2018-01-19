/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#include <ignition/transport/log/QueryOptions.hh>

using namespace ignition::transport;
using namespace ignition::transport::log;

//////////////////////////////////////////////////
/// \internal Implementation for QualifiedTime
class ignition::transport::log::QualifiedTime::Implementation
{
  // See QualifiedTime()
  public: Implementation()
    : indeterminate(true)
  {
    // Do nothing
  }

  // See QualifiedTime(const std::chrono::nanoseconds&, Qualifier)
  public: Implementation(const Time &_time,
                         Qualifier _qualifier)
    : indeterminate(false),
      qualifier(_qualifier),
      time(_time)
  {
    // Do nothing
  }

  // See QualifiedTime::IsIndeterminate()
  public: bool IsIndeterminate() const
  {
    return indeterminate;
  }

  // See QualifiedTime::GetTime()
  public: const std::chrono::nanoseconds *GetTime() const
  {
    if (indeterminate)
      return nullptr;

    return &time;
  }

  // See QualifiedTime::GetQualifier()
  public: const Qualifier *GetQualifier() const
  {
    if (indeterminate)
      return nullptr;

    return &qualifier;
  }

  // See QualifiedTime::SetTime()
  public: void SetTime(const std::chrono::nanoseconds &_time,
                       Qualifier _qualifier)
  {
    indeterminate = false;
    time = _time;
    qualifier = _qualifier;
  }

  // See QualifiedTime::Clear()
  public: void Clear()
  {
    indeterminate = true;
  }

  /// \internal Flag to keep track of whether this QualifiedTime is indeterminate
  private: bool indeterminate;

  /// \internal Qualifier for the QualifiedTime, if it is not indeterminate
  private: Qualifier qualifier;

  /// \internal Time value for the QualifiedTime, if it is not indeterminate
  private: std::chrono::nanoseconds time;
};

//////////////////////////////////////////////////
QualifiedTime::QualifiedTime()
  : dataPtr(new Implementation)
{
  // Do nothing
}


//////////////////////////////////////////////////
QualifiedTime::~QualifiedTime()
{
  // Destroy pimpl
}
