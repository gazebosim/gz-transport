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
#include <ignition/msgs/statistic.pb.h>

#include <chrono>
#include <cmath>
#include <sstream>

#include "ignition/transport/TopicStatistics.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
void Statistics::Update(double _stat)
{
  // Increase the sample count.
  this->count++;

  // Update the rolling average
  const double currentAvg = this->average;
  this->average = currentAvg +
    (_stat - currentAvg) / this->count;

  // Store the min and max.
  this->min = std::min(this->min, _stat);
  this->max = std::max(this->max, _stat);

  // Update the variance, used to calculate the standard deviation,
  // using Welford's algorithm described at
  // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford%27s_online_algorithm
  this->sumSquareMeanDist += (_stat - currentAvg) *
    (_stat - this->average);
}

//////////////////////////////////////////////////
double Statistics::Avg() const
{
  return this->average;
}

//////////////////////////////////////////////////
double Statistics::StdDev() const
{
  return this->count > 0 ? std::sqrt(this->sumSquareMeanDist / this->count) : 0;
}

//////////////////////////////////////////////////
double Statistics::Min() const
{
  return this->min;
}

//////////////////////////////////////////////////
double Statistics::Max() const
{
  return this->max;
}

//////////////////////////////////////////////////
uint64_t Statistics::Count() const
{
  return this->count;
}

//////////////////////////////////////////////////
void TopicStatistics::Update(const std::string &_sender,
    uint64_t _stamp, uint64_t _seq)
{
  // Current wall time
  uint64_t now =
    std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

  if (this->prevPublicationStamp != 0)
  {
    this->publication.Update(_stamp - this->prevPublicationStamp);
    this->reception.Update(now - this->prevReceptionStamp);
    this->age.Update(now - _stamp);

    if (this->seq[_sender] + 1 != _seq)
    {
      this->droppedMsgCount++;
    }
  }

  this->prevPublicationStamp = _stamp;
  this->prevReceptionStamp = now;

  this->seq[_sender] = _seq;
}

//////////////////////////////////////////////////
void TopicStatistics::FillMessage(msgs::Metric &_msg) const
{
  _msg.set_unit("milliseconds");
  msgs::Statistic *stat = _msg.add_statistics();
  stat->set_type(msgs::Statistic::SAMPLE_COUNT);
  stat->set_name("dropped_message_count");
  stat->set_value(this->droppedMsgCount);

  // Publication statistics
  msgs::StatisticsGroup *statGroup = _msg.add_statistics_groups();
  statGroup->set_name("publication_statistics");
  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::AVERAGE);
  stat->set_name("avg_hz");
  stat->set_value(1.0 / this->publication.Avg());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::MINIMUM);
  stat->set_name("min_period");
  stat->set_value(this->publication.Min());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::MAXIMUM);
  stat->set_name("max_period");
  stat->set_value(this->publication.Max());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::STDDEV);
  stat->set_name("period_standard_devation");
  stat->set_value(this->publication.StdDev());

  // Reception statistics
  statGroup = _msg.add_statistics_groups();
  statGroup->set_name("reception_statistics");

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::AVERAGE);
  stat->set_name("avg_hz");
  stat->set_value(1.0 / this->reception.Avg());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::MINIMUM);
  stat->set_name("min_period");
  stat->set_value(this->reception.Min());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::MAXIMUM);
  stat->set_name("max_period");
  stat->set_value(this->reception.Max());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::STDDEV);
  stat->set_name("period_standard_devation");
  stat->set_value(this->reception.StdDev());

  // Age statistics
  statGroup = _msg.add_statistics_groups();
  statGroup->set_name("age_statistics");

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::AVERAGE);
  stat->set_name("avg_age");
  stat->set_value(this->age.Avg());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::MINIMUM);
  stat->set_name("min_age");
  stat->set_value(this->age.Min());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::MAXIMUM);
  stat->set_name("max_age");
  stat->set_value(this->age.Max());

  stat = statGroup->add_statistics();
  stat->set_type(msgs::Statistic::STDDEV);
  stat->set_name("age_standard_devation");
  stat->set_value(this->age.StdDev());
}

//////////////////////////////////////////////////
uint64_t TopicStatistics::DroppedMsgCount() const
{
  return this->droppedMsgCount;
}

//////////////////////////////////////////////////
Statistics TopicStatistics::PublicationStatistics() const
{
  return this->publication;
}

//////////////////////////////////////////////////
Statistics TopicStatistics::ReceptionStatistics() const
{
  return this->reception;
}

//////////////////////////////////////////////////
Statistics TopicStatistics::AgeStatistics() const
{
  return this->age;
}
