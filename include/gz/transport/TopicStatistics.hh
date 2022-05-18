/*
 * Copyright (C) 2020 Open Source Robotics Foundation
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
#ifndef GZ_TRANSPORT_TOPICSTATISTICS_HH_
#define GZ_TRANSPORT_TOPICSTATISTICS_HH_

#include <gz/msgs/statistic.pb.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include "gz/transport/config.hh"
#include "gz/transport/Export.hh"

#ifdef _WIN32
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#ifdef min
  #undef min
  #undef max
#endif
#include <windows.h>
#endif

namespace gz
{
  namespace transport
  {
    // Inline bracket to help doxygen filtering.
    inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
    //
    // Forward declarations.
    class TopicStatisticsPrivate;

    /// \brief Computes the rolling average, min, max, and standard
    /// deviation for a set of samples.
    class IGNITION_TRANSPORT_VISIBLE Statistics
    {
      /// \brief Default constructor.
      public: Statistics() = default;

      /// \brief Default destructor.
      public: ~Statistics() = default;

      /// \brief Update with a new sample.
      /// \param[in] _stat New statistic sample.
      public: void Update(double _stat);

      /// \brief Get the average value.
      /// \return the average value.
      public: double Avg() const;

      /// \brief Get the standard deviation.
      /// \return The standard deviation.
      public: double StdDev() const;

      /// \brief Get the minimum sample value.
      /// \return The minimum sample value.
      public: double Min() const;

      /// \brief Get the maximum sample value.
      /// \return The maximum sample value.
      public: double Max() const;

      /// \brief Get the number of samples.
      /// \return The number of samples.
      public: uint64_t Count() const;

      /// \brief Count of the samples.
      private: uint64_t count = 0;

      /// \brief Average value.
      private: double average = 0;

      /// \brief Sum of the squared mean distance between samples. This is
      /// used to calculate the standard deviation.
      private: double sumSquareMeanDist = 0;

      /// \brief Minimum sample.
      private: double min = std::numeric_limits<double>::max();

      /// \brief Maximum sample.
      private: double max = std::numeric_limits<double>::min();
    };

    /// \brief Encapsulates statistics for a single topic. The set of
    /// statistics include:
    ///
    /// 1. Number of dropped messages.
    ///    max time between publications.
    /// 3. Receive statistics: The reception hz rate, standard
    ///    deviation between receiving messages, min time between receiving
    ///    messages, and max time between receiving messages.
    ///
    /// Publication statistics utilize time stamps generated by the
    /// publisher. Receive statistics use time stamps generated by the
    /// subscriber.
    class IGNITION_TRANSPORT_VISIBLE TopicStatistics
    {
      /// \brief Default constructor.
      public: TopicStatistics();

      /// \brief Copy constructor.
      /// \param[in] _stats Statistics to copy.
      public: TopicStatistics(const TopicStatistics &_stats);

      /// \brief Default destructor.
      public: ~TopicStatistics();

      /// \brief Update the topic statistics.
      /// \param[in] _sender Address of the sender.
      /// \param[in] _stamp Publication time stamp.
      /// \param[in] _seq Publication sequence number.
      public: void Update(const std::string &_sender,
                          uint64_t _stamp, uint64_t _seq);

      /// \brief Populate an gz::msgs::Metric message with topic
      /// statistics.
      /// \param[in] _msg Message to populate.
      public: void FillMessage(msgs::Metric &_msg) const;

      /// \brief Get the number of dropped messages.
      /// \return Number of dropped messages.
      public: uint64_t DroppedMsgCount() const;

      /// \brief Get statistics about publication of messages.
      /// \return Publication statistics.
      public: Statistics PublicationStatistics() const;

      /// \brief Get the statistics about reception of messages.
      /// \return Reception statistics.
      public: Statistics ReceptionStatistics() const;

      /// \brief Get the message age statistics.
      /// \return Age statistics.
      public: Statistics AgeStatistics() const;
#ifdef _WIN32
// Disable warning C4251 which is triggered by
// std::unique_ptr
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
      /// \brief Private data pointer.
      private: std::unique_ptr<TopicStatisticsPrivate> dataPtr;
#ifdef _WIN32
#pragma warning(pop)
#endif
    };
    }
  }
}
#endif