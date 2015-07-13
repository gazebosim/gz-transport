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

#include "ignition/transport/TopicStorage.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
bool TopicStorage::AddPublisher(const Publisher &_publisher)
{
  // The topic does not exist.
  if (this->data.find(_publisher.Topic()) == this->data.end())
  {
    // VS2013 is buggy with initializer list here {}
    this->data[_publisher.Topic()] =
      std::map<std::string, std::vector<Publisher>>();
  }

  // Check if the process uuid exists.
  auto &m = this->data[_publisher.Topic()];
  if (m.find(_publisher.PUuid()) != m.end())
  {
    // Check that the Publisher does not exist.
    auto &v = m[_publisher.PUuid()];
    auto found = std::find_if(v.begin(), v.end(),
      [&](const Publisher &_pub)
      {
        return _pub.Addr()  == _publisher.Addr() &&
               _pub.NUuid() == _publisher.NUuid();
      });

    // The publisher was already existing, just exit.
    if (found != v.end())
      return false;
  }

  // Add a new Publisher entry.
  m[_publisher.PUuid()].push_back(Publisher(_publisher));
  return true;
}

//////////////////////////////////////////////////
bool TopicStorage::HasTopic(const std::string &_topic)
{
  return this->data.find(_topic) != this->data.end();
}

//////////////////////////////////////////////////
bool TopicStorage::HasAnyPublishers(const std::string &_topic,
                                    const std::string &_pUuid)
{
  if (!this->HasTopic(_topic))
    return false;

  return this->data[_topic].find(_pUuid) != this->data[_topic].end();
}

//////////////////////////////////////////////////
bool TopicStorage::HasPublisher(const std::string &_addr)
{
  for (auto &topic : this->data)
  {
    for (auto &proc : topic.second)
    {
      for (auto &pub : proc.second)
      {
        if (pub.Addr() == _addr)
          return true;
      }
    }
  }
  return false;
}

//////////////////////////////////////////////////
bool TopicStorage::GetPublisher(const std::string &_topic,
                                const std::string &_pUuid,
                                const std::string &_nUuid,
                                Publisher &_publisher)
{
  // Topic not found.
  if (this->data.find(_topic) == this->data.end())
    return false;

  // m is {pUUID=>Publisher}.
  auto &m = this->data[_topic];

  // pUuid not found.
  if (m.find(_pUuid) == m.end())
    return false;

  // Vector of 0MQ known addresses for a given topic and pUuid.
  auto &v = m[_pUuid];
  auto found = std::find_if(v.begin(), v.end(),
    [&](const Publisher &_pub)
    {
      return _pub.NUuid() == _nUuid;
    });
  // Address found!
  if (found != v.end())
  {
    _publisher = *found;
    return true;
  }

  // nUuid not found.
  return false;
}

//////////////////////////////////////////////////
bool TopicStorage::GetPublishers(const std::string &_topic,
                                 std::map<std::string,
                                    std::vector<Publisher>> &_info)
{
  if (!this->HasTopic(_topic))
    return false;

  _info = this->data[_topic];
  return true;
}

//////////////////////////////////////////////////
bool TopicStorage::DelPublisherByNode(const std::string &_topic,
                                      const std::string &_pUuid,
                                      const std::string &_nUuid)
{
  unsigned int counter = 0;

  // Iterate over all the topics.
  if (this->data.find(_topic) != this->data.end())
  {
    // m is {pUUID=>Publisher}.
    auto &m = this->data[_topic];

    // The pUuid exists.
    if (m.find(_pUuid) != m.end())
    {
      // Vector of 0MQ known addresses for a given topic and pUuid.
      auto &v = m[_pUuid];
      auto priorSize = v.size();
      v.erase(std::remove_if(v.begin(), v.end(),
        [&](const Publisher &_pub)
        {
          return _pub.NUuid() == _nUuid;
        }),
        v.end());
      counter = priorSize - v.size();

      if (v.empty())
        m.erase(_pUuid);

      if (m.empty())
        this->data.erase(_topic);
    }
  }

  return counter > 0;
}

//////////////////////////////////////////////////
bool TopicStorage::DelPublishersByProc(const std::string &_pUuid)
{
  unsigned int counter = 0;

  // Iterate over all the topics.
  for (auto it = this->data.begin(); it != this->data.end();)
  {
    // m is {pUUID=>Publisher}.
    auto &m = it->second;
    counter = m.erase(_pUuid);
    if (m.empty())
      this->data.erase(it++);
    else
      ++it;
  }

  return counter > 0;
}

//////////////////////////////////////////////////
void TopicStorage::GetPublishersByProc(const std::string &_pUuid,
                                   std::map<std::string,
                                      std::vector<Publisher>> &_pubs)
{
  _pubs.clear();

  // Iterate over all the topics.
  for (auto &topic : this->data)
  {
    // m is {pUUID=>Publisher}.
    auto &m = topic.second;
    if (m.find(_pUuid) != m.end())
    {
      auto &v = m[_pUuid];
      for (auto &pub : v)
      {
        _pubs[topic.first].push_back(Publisher(pub));
      }
    }
  }
}

//////////////////////////////////////////////////
void TopicStorage::GetTopicList(std::vector<std::string> &_topics) const
{
  for (auto &topic : this->data)
    _topics.push_back(topic.first);
}

//////////////////////////////////////////////////
void TopicStorage::Print()
{
  std::cout << "---" << std::endl;
  for (auto &topic : this->data)
  {
    std::cout << "[" << topic.first << "]" << std::endl;
    auto &m = topic.second;
    for (auto &proc : m)
    {
      std::cout << "\tProc. UUID: " << proc.first << std::endl;
      auto &v = proc.second;
      for (auto &publisher : v)
      {
        std::cout << publisher;
      }
    }
  }
}
