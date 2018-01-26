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

#include <ignition/transport/log/QualifiedTime.hh>

using namespace ignition::transport::log;

//////////////////////////////////////////////////
/// \internal Implementation for QualifiedTime
class ignition::transport::log::QualifiedTime::Implementation
{
  /// \internal \sa QualifiedTime(const std::chrono::nanoseconds&, Qualifier)
  public: Implementation(const Time &_time,
                         Qualifier _qualifier)
    : indeterminate(false),
      qualifier(_qualifier),
      time(_time)
  {
    // Do nothing
  }

  /// \internal \sa QualifiedTime()
  public: Implementation()
    : indeterminate(true)
  {
    // Do nothing
  }

  /// \internal \sa QualifiedTime::IsIndeterminate()
  public: bool IsIndeterminate() const
  {
    return this->indeterminate;
  }

  /// \internal \sa QualifiedTime::GetTime()
  public: const std::chrono::nanoseconds *GetTime() const
  {
    if (this->indeterminate)
      return nullptr;

    return &this->time;
  }

  /// \internal \sa QualifiedTime::GetQualifier()
  public: const Qualifier *GetQualifier() const
  {
    if (this->indeterminate)
      return nullptr;

    return &this->qualifier;
  }

  /// \internal \sa QualifiedTime::SetTime()
  public: void SetTime(const std::chrono::nanoseconds &_time,
                       Qualifier _qualifier)
  {
    this->indeterminate = false;
    this->time = _time;
    this->qualifier = _qualifier;
  }

  /// \internal \sa QualifiedTime::Clear()
  public: void Clear()
  {
    this->indeterminate = true;
  }

  /// \internal Flag to keep track of whether this QualifiedTime is
  /// indeterminate
  private: bool indeterminate;

  /// \internal Qualifier for the QualifiedTime, if it is not indeterminate
  private: Qualifier qualifier;

  /// \internal Time value for the QualifiedTime, if it is not indeterminate
  private: std::chrono::nanoseconds time;
};

//////////////////////////////////////////////////
QualifiedTime::QualifiedTime(const Time &_time,
                             Qualifier _qualifier)
  : dataPtr(new Implementation(_time, _qualifier))
{
  // Do nothing
}

//////////////////////////////////////////////////
QualifiedTime::QualifiedTime()
  : dataPtr(new Implementation)
{
  // Do nothing
}

//////////////////////////////////////////////////
QualifiedTime::QualifiedTime(const QualifiedTime &_other)
  : dataPtr(new Implementation(*_other.dataPtr))
{
  // Do nothing
}

//////////////////////////////////////////////////
QualifiedTime & QualifiedTime::operator=(const QualifiedTime &_other)
{
  *this->dataPtr = *_other.dataPtr;
  return *this;
}

//////////////////////////////////////////////////
bool QualifiedTime::IsIndeterminate() const
{
  return this->dataPtr->IsIndeterminate();
}

//////////////////////////////////////////////////
auto QualifiedTime::GetTime() const -> const Time *
{
  return this->dataPtr->GetTime();
}

//////////////////////////////////////////////////
auto QualifiedTime::GetQualifier() const -> const Qualifier *
{
  return this->dataPtr->GetQualifier();
}

//////////////////////////////////////////////////
void QualifiedTime::SetTime(const Time &_time,
                            Qualifier _qualifier)
{
  this->dataPtr->SetTime(_time, _qualifier);
}

//////////////////////////////////////////////////
void QualifiedTime::Clear()
{
  this->dataPtr->Clear();
}

//////////////////////////////////////////////////
QualifiedTime::~QualifiedTime()
{
  // Destroy pimpl
}

//////////////////////////////////////////////////
class ignition::transport::log::QualifiedTimeRange::Implementation
{
  /// \internal \sa QualifiedTimeRange()
  public: Implementation(const QualifiedTime &_start,
                         const QualifiedTime &_finish)
    : start(_start),
      finish(_finish)
  {
    // Do nothing
  }

  /// \internal \sa QualifiedTimeRange::GetStart()
  const QualifiedTime &GetStart() const
  {
    return this->start;
  }

  /// \internal \sa QualifiedTimeRange::GetFinish()
  const QualifiedTime &GetFinish() const
  {
    return this->finish;
  }

  /// \internal \sa QualifiedTimeRange::SetStart()
  bool SetStart(const QualifiedTime &_start)
  {
    this->start = _start;
    return this->Valid();
  }

  /// \internal \sa QualifiedTimeRange::SetFinish()
  bool SetFinish(const QualifiedTime &_finish)
  {
    this->finish = _finish;
    return this->Valid();
  }

  /// \internal \sa QualifiedTimeRange::SetRange()
  bool SetRange(const QualifiedTime &_start,
                const QualifiedTime &_finish)
  {
    this->start = _start;
    this->finish = _finish;
    return this->Valid();
  }

  /// \internal \sa QualifiedTimeRange::Valid()
  bool Valid() const
  {
    // If the start is indeterminate, then the range is certainly valid.
    const QualifiedTime::Time *ts = this->start.GetTime();
    if (nullptr == ts)
      return true;

    // If the finish is indeterminate, then the range is certainly valid.
    const QualifiedTime::Time *tf = this->finish.GetTime();
    if (nullptr == tf)
      return true;

    // If the start is less than or equal to the finish, the range is valid.
    return (*ts <= *tf);
  }

  /// \brief The time where the range starts
  public: QualifiedTime start;

  /// \brief The time where the range finishes
  public: QualifiedTime finish;
};

//////////////////////////////////////////////////
QualifiedTimeRange::QualifiedTimeRange(
    const QualifiedTime &_start,
    const QualifiedTime &_finish)
  : dataPtr(new Implementation(_start, _finish))
{
  // Do nothing
}

//////////////////////////////////////////////////
QualifiedTimeRange::QualifiedTimeRange(const QualifiedTimeRange &_other)
  : dataPtr(new Implementation(*_other.dataPtr))
{
  // Do nothing
}

//////////////////////////////////////////////////
QualifiedTimeRange &QualifiedTimeRange::operator=(
    const QualifiedTimeRange &_other)
{
  *this->dataPtr = *_other.dataPtr;
  return *this;
}

//////////////////////////////////////////////////
QualifiedTimeRange QualifiedTimeRange::From(const QualifiedTime &_start)
{
  return QualifiedTimeRange(_start, QualifiedTime());
}

//////////////////////////////////////////////////
QualifiedTimeRange QualifiedTimeRange::Until(const QualifiedTime &_finish)
{
  return QualifiedTimeRange(QualifiedTime(), _finish);
}

//////////////////////////////////////////////////
QualifiedTimeRange QualifiedTimeRange::AllTime()
{
  return QualifiedTimeRange(QualifiedTime(), QualifiedTime());
}

//////////////////////////////////////////////////
const QualifiedTime &QualifiedTimeRange::GetStart() const
{
  return this->dataPtr->GetStart();
}

//////////////////////////////////////////////////
const QualifiedTime &QualifiedTimeRange::GetFinish() const
{
  return this->dataPtr->GetFinish();
}

//////////////////////////////////////////////////
bool QualifiedTimeRange::SetStart(const QualifiedTime &_start)
{
  return this->dataPtr->SetStart(_start);
}

//////////////////////////////////////////////////
bool QualifiedTimeRange::SetFinish(const QualifiedTime &_finish)
{
  return this->dataPtr->SetFinish(_finish);
}

//////////////////////////////////////////////////
bool QualifiedTimeRange::SetRange(const QualifiedTime &_start,
                                  const QualifiedTime &_finish)
{
  return this->dataPtr->SetRange(_start, _finish);
}

//////////////////////////////////////////////////
bool QualifiedTimeRange::Valid() const
{
  return this->dataPtr->Valid();
}

//////////////////////////////////////////////////
QualifiedTimeRange::~QualifiedTimeRange()
{
  // Destroy pimpl
}
