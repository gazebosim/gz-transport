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

#ifndef IGNITION_TRANSPORT_QUERYOPTIONS_HH_
#define IGNITION_TRANSPORT_QUERYOPTIONS_HH_

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <regex>

#include <ignition/transport/log/Export.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      //////////////////////////////////////////////////
      /// \brief The QueryOptions interface is used by Log::QueryMessages() to
      /// determine which messages are retrieved from the log file.
      class IGNITION_TRANSPORT_LOG_VISIBLE QueryOptions
      {
        /// \brief Generate one or more SQL query statements to be used by the
        /// log file to produce a Batch of messages.
        /// \return One or more SQL statements.
        public: virtual std::vector<std::string> GenerateStatements() const = 0;

        /// \brief Virtual destructor
        public: virtual ~QueryOptions() = default;
      };

      //////////////////////////////////////////////////
      /// \brief Since time is coninuous, it may be difficult to know the exact
      /// time stamp of a desired message. The QualifiedTime class provides a
      /// way to tailor how a time stamp is interpreted by the message query.
      ///
      /// Note that the value of this time object may be interpreted as a
      /// relative time or as an absolute time stamp depending on the context in
      /// which it gets used.
      class IGNITION_TRANSPORT_LOG_VISIBLE QualifiedTime
      {
        /// \brief The Qualifier determines the behavior of how a message is
        /// selected.
        public: enum Qualifier
        {
          /// \brief Get either an exact time match, or the closest time before
          /// the specified time stamp.
          OrClosestBefore = 0,

          /// \brief Get the closest time before the specified time stamp.
          ClosestBefore,

          /// \brief Get either an exact time match, or the closest time after
          /// the specified time stamp.
          OrClosestAfter,

          /// \brief Get the closest time after the specified time stamp.
          ClosestAfter
        };

        /// \brief Convenient typedef for our time representation. Note that any
        /// std::chrono::duration type can be implicitly converted to this type.
        /// E.g. you can pass a
        ///
        /// \code
        /// std::chrono::seconds(value)
        /// std::chrono::milliseconds(value)
        /// std::chrono::minutes(value)
        /// // ... ect ...
        /// \endcode
        ///
        /// into any function that accepts this Time type.
        using Time = std::chrono::nanoseconds;

        /// \brief Construct a qualified time specifier.
        /// \param[in] _time The time stamp that is used as the focal point of
        /// this qualified time.
        /// \param[in] _qualifier The qualifier that determines the exact
        /// interpretation of the _time value.
        public: QualifiedTime(const Time &_time,
                              Qualifier _qualifier = OrClosestBefore);

        /// \brief Default constructor. The time will be treated as
        /// indeterminate. This means that the QualifiedTime object will be
        /// taken to indicate that no time has been specified at all.
        /// \sa IsIndeterminate()
        public: QualifiedTime( /* Indeterminate */ );

        /// \brief Indicates whether this QualifiedTime object is indeterminate.
        ///
        /// When an indeterminate time is used as the end of a range, it is
        /// taken to mean that the range will go on endlessly. When used as the
        /// beginning of a range, it means that the range should extend as far
        /// into the past as possible.
        ///
        /// \return true if this QualifiedTime is indeterminate, or false if it
        /// does have a determined time.
        public: bool IsIndeterminate() const;

        /// \brief Get the time stamp for this qualified time, unless the time
        /// is indeterminate.
        /// \return A pointer to the time value that is specified by this
        /// QualifiedTime object, or a nullptr if this QualifiedTime is
        /// indeterminate.
        public: const Time *GetTime() const;

        /// \brief Get the qualifier for this qualified time, unless the time
        /// is indeterminate.
        /// \return A pointer to the qualifier that is specified by this
        /// QualifiedTime object, or a nullptr if this QualifiedTime is
        /// indeterminate.
        public: const Time *GetQualifier() const;

        /// \brief Set the time that this QualifiedTime object represents.
        /// \param[in] _time The time stamp that is used as the focal point of
        /// this qualified time.
        /// \param[in] _qualifier The qualifier that determines the exact
        /// interpretation of the _time value.
        public: void SetTime(const Time &_time,
                             Qualifier _qualifier = OrClosestBefore);

        /// \brief Set this QualifiedTime object to be indeterminate.
        /// \sa IsIndeterminate()
        void Clear();

        /// \brief Destructor
        public: ~QualifiedTime();

        /// \internal Implementation class
        private: class Implementation;

        /// \internal PIMPL pointer
        private: std::unique_ptr<Implementation> dataPtr;
      };

      //////////////////////////////////////////////////
      /// \brief The QualifiedTimeRange class provides a pair of qualified times
      /// that represent a range. This is used to specify a desired time range
      /// to the BasicQueryOptions class.
      class IGNITION_TRANSPORT_LOG_VISIBLE QualifiedTimeRange
      {
        /// \brief Construct a time range.
        /// \param[in] _start The beginning of the time range.
        /// \param[in] _finish The end of the time range.
        /// \sa From()
        /// \sa Until()
        /// \sa AllTime()
        public: QualifiedTimeRange(
          const QualifiedTime &_start,
          const QualifiedTime &_finish);

        /// \brief Construct a time range that begins at _start and never ends.
        /// \param[in] _start The beginning of the time range.
        /// \return A time range that begins at _start and never ends.
        public: static QualifiedTimeRange From(const QualifiedTime &_start);

        /// \brief Construct a time range that ends at _finish and has no
        /// beginning.
        /// \param[in] _finish The end of the time range.
        /// \return A time range that ends at _finish and has no beginning.
        public: static QualifiedTimeRange Until(const QualifiedTime &_finish);

        /// \brief Construct a time range that has no beginning or end.
        /// \return A time range that has no beginning or end.
        public: static QualifiedTimeRange AllTime();

        /// \brief Get a reference to the start time of this range.
        /// \return A reference to the start time.
        public: const QualifiedTime &GetStart() const;

        /// \brief Get a reference to the end time of this range.
        /// \return A reference to the end time.
        public: const QualifiedTime &GetFinish() const;

        /// \brief Set the start time of this range. Passing in an indeterminate
        /// QualifiedTime (its default constructor) will tell this range to have
        /// no beginning (effectively, it should start at the beginning of the
        /// log).
        /// \param[in] _start The start time of this range.
        /// \return True if the new range is valid. False if the range is now
        /// invalid. The _start value will be accepted either way.
        /// \sa SetRange()
        /// \sa Valid()
        public: bool SetStart(const QualifiedTime &_start);

        /// \brief Set the finish time of this range. Passing in an
        /// indeterminate QualifiedTime (its default constructor) will tell this
        /// range to have no end (effectively, it should not stop until the end
        /// of the log).
        /// \param[in] _finish The finish time of this range.
        /// \return True if this new range is valid. False if the range is now
        /// invalid. The _finish value will be accepted either way.
        /// \sa SetRange()
        /// \sa Valid()
        public: bool SetFinish(const QualifiedTime &_finish);

        /// \brief Set both endpoints of the range.
        /// \param[in] _start The start time of this range.
        /// \param[in] _finish The end time of this range.
        /// \return True if this new range is valid. False if the range is now
        /// invalid. The values for _start and _finish will be accepted either
        /// way.
        /// \sa SetStart()
        /// \sa SetFinish()
        /// \sa Valid()
        public: bool SetRange(const QualifiedTime &_start,
                              const QualifiedTime &_finish);

        /// \brief Check if the range is valid. A valid range means that the
        /// finish time is guaranteed to be later than or coincident with the
        /// start time.
        /// \return True if the range is valid, false if invalid.
        public: bool Valid() const;

        /// \brief Virtual destructor
        public: ~QualifiedTimeRange();

        /// \internal Implementation class
        private: class Implementation;

        /// \internal PIMPL pointer
        private: std::unique_ptr<Implementation> dataPtr;
      };

      //////////////////////////////////////////////////
      /// \brief The BasicQueryOptions class provides a basic implementation of
      /// QueryOptions which allows messages to be queried based on topic name
      /// and time received.
      class IGNITION_TRANSPORT_LOG_VISIBLE BasicQueryOptions final
          : public virtual QueryOptions
      {
        /// \brief Query for all topics over the specified time range (by
        /// default, all time).
        /// \param[in] _range The time range to query over.
        public: BasicQueryOptions(
          const QualifiedTimeRange &_range = QualifiedTimeRange::AllTime());

        /// \brief Query a single topic over the specified time range (by
        /// default, all time).
        public: BasicQueryOptions(
          const std::string &_topic,
          const QualifiedTimeRange &_range = QualifiedTimeRange::AllTime());

        /// \brief Query for messages belonging to all topics that match a
        /// pattern, over the specified time range (by default, all time).
        public: BasicQueryOptions(
          const std::regex &_pattern,
          const QualifiedTimeRange &_range = QualifiedTimeRange::AllTime());

        // Documentation inherited
        public: std::vector<std::string> GenerateStatements() const override;

        // Virtual destructor
        public: ~BasicQueryOptions();

        /// \internal Implementation class
        private: class Implementation;

        /// \internal PIMPL pointer
        private: std::unique_ptr<Implementation> dataPtr;
      };
    }
  }
}

#endif
