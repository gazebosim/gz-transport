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
class TimeRangeOption::Implementation
{
  /// \brief Range for this option
  public: QualifiedTimeRange range;
};

//////////////////////////////////////////////////
TimeRangeOption::TimeRangeOption(const QualifiedTimeRange &_timeRange)
  : dataPtr(new Implementation{_timeRange})
{
  // Do nothing
}

//////////////////////////////////////////////////
QualifiedTimeRange &TimeRangeOption::TimeRange()
{
  return this->dataPtr->range;
}

//////////////////////////////////////////////////
const QualifiedTimeRange &TimeRangeOption::TimeRange() const
{
  return this->dataPtr->range;
}

//////////////////////////////////////////////////
TimeRangeOption::~TimeRangeOption()
{
  // Destroy the pimpl
}

//////////////////////////////////////////////////
class TopicList::Implementation
{
  /// \brief Topics for this option
  public: std::set<std::string> topics;
};

//////////////////////////////////////////////////
TopicList::TopicList(
    const std::set<std::string> &_topics,
    const QualifiedTimeRange &_timeRange)
  : TimeRangeOption(_timeRange),
    dataPtr(new Implementation{_topics})
{
  // Do nothing
}

//////////////////////////////////////////////////
TopicList::TopicList(
    const std::string &_singleTopic,
    const QualifiedTimeRange &_timeRange)
  : TopicList(std::set<std::string>{_singleTopic}, _timeRange)
{
  // Do nothing
}

//////////////////////////////////////////////////
std::set<std::string> &TopicList::Topics()
{
  return this->dataPtr->topics;
}

//////////////////////////////////////////////////
const std::set<std::string> &TopicList::Topics() const
{
  return this->dataPtr->topics;
}

//////////////////////////////////////////////////
std::vector<SqlStatement> TopicList::GenerateStatements(
    const Descriptor &_descriptor) const
{
  // TODO(SQL)
}

//////////////////////////////////////////////////
TopicList::~TopicList()
{
  // Destroy the pimpl
}

//////////////////////////////////////////////////
class TopicPattern::Implementation
{
  /// \brief Pattern for this option
  /// TODO: Consider making this a vector of patterns?
  public: std::regex pattern;
};

//////////////////////////////////////////////////
TopicPattern::TopicPattern(
    const std::regex &_pattern,
    const QualifiedTimeRange &_timeRange)
  : TimeRangeOption(_timeRange),
    dataPtr(new Implementation{_pattern})
{
  // Do nothing
}

//////////////////////////////////////////////////
std::regex &TopicPattern::Pattern()
{
  return this->dataPtr->pattern;
}

//////////////////////////////////////////////////
const std::regex &TopicPattern::Pattern() const
{
  return this->dataPtr->pattern;
}

//////////////////////////////////////////////////
std::vector<SqlStatement> TopicPattern::GenerateStatements(
    const Descriptor &_descriptor) const
{
  // TODO(SQL)
}

//////////////////////////////////////////////////
TopicPattern::~TopicPattern()
{
  // Destroy the pimpl
}

//////////////////////////////////////////////////
class AllTopics::Implementation
{
  // Empty for now
};

//////////////////////////////////////////////////
AllTopics::AllTopics(const QualifiedTimeRange &_timeRange)
  : TimeRangeOption(_timeRange)
{
  // Do nothing. We don't even allocate the PIMPL because we have no use for it
  // yet. We might never need it, but we'll hang onto it just in case the needs
  // of this class change in the future.
}

//////////////////////////////////////////////////
std::vector<SqlStatement> AllTopics::GenerateStatements(
    const Descriptor &_descriptor) const
{
  // TODO(SQL)
}

//////////////////////////////////////////////////
AllTopics::~AllTopics()
{
  // Destroy the pimpl
}
