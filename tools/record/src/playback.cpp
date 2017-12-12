/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <ignition/transport.hh>
#include <sqlite3.h>

#include "config.hh"
#include "src/raii-sqlite3.hh"


typedef struct
{
  std::size_t id;
  std::string name;
  std::string type;
} KnownTopic;

typedef struct
{
  int64_t sec;
  int64_t nano;
} Time;

typedef struct
{
  Time time;
  KnownTopic topic;
  std::string message;
} ReceivedMessage;


//////////////////////////////////////////////////
/// \brief Return true if two KnownTopic's are identical
bool operator==(const KnownTopic &_lhs, const KnownTopic &_rhs)
{
  return _lhs.id == _rhs.id;
}

//////////////////////////////////////////////////
/// \brief allow KnownTopic to be a key in a map
namespace std {
  template <> struct hash<KnownTopic>
  {
    size_t operator()(const KnownTopic &_topic) const
    {
      return _topic.id;
    }
  };
}

//////////////////////////////////////////////////
/// \brief Get duration between two times
std::chrono::nanoseconds operator-(const Time &_lhs, const Time &_rhs)
{
  std::chrono::seconds lhs_sec(_lhs.sec);
  std::chrono::nanoseconds lhs_nsec(_lhs.nano);
  std::chrono::seconds rhs_sec(_rhs.sec);
  std::chrono::nanoseconds rhs_nsec(_rhs.nano);

  std::chrono::nanoseconds lhs_tot = std::chrono::nanoseconds(lhs_sec) + lhs_nsec;
  std::chrono::nanoseconds rhs_tot = std::chrono::nanoseconds(rhs_sec) + rhs_nsec;

  return lhs_tot - rhs_tot;
}

//////////////////////////////////////////////////
std::string checkVersion(raii_sqlite3::Database &db)
{
  // Compile the statement
  const char *get_version = "SELECT to_version FROM migrations ORDER BY id DESC LIMIT 1;";
  raii_sqlite3::Statement statement(db, get_version);
  if (!statement)
  {
    std::cerr << "Failed to compile statement\n";
    return "";
  }

  // Try to run it
  int result_code = sqlite3_step(statement.Handle());
  if (result_code != SQLITE_ROW)
  {
    std::cerr << "Database has no version\n";
    return "";
  }

  // Version is free'd automatically when statement is destructed
  const unsigned char *version = sqlite3_column_text(statement.Handle(), 0);
  return std::string(reinterpret_cast<const char *>(version));
};

//////////////////////////////////////////////////
bool getTopics(raii_sqlite3::Database &db, std::vector<KnownTopic> &_topics)
{
  // Compile the statement
  const char *get_version = "SELECT topics.id, topics.name, message_types.name FROM topics JOIN message_types ON topics.message_type_id = message_types.id;";
  raii_sqlite3::Statement statement(db, get_version);
  if (!statement)
  {
    std::cerr << "Failed to compile statement\n";
    return false;
  }

  // Get the results from the statement
  while (SQLITE_ROW == sqlite3_step(statement.Handle()))
  {
    std::size_t id = sqlite3_column_int64(statement.Handle(), 0);
    const unsigned char *name = sqlite3_column_text(statement.Handle(), 1);
    const unsigned char *type = sqlite3_column_text(statement.Handle(), 2);

    KnownTopic topic;
    topic.id = id;
    topic.name = reinterpret_cast<const char *>(name);
    topic.type = reinterpret_cast<const char *>(type);
    _topics.push_back(topic);
  }
  return true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Check arguments
  if (argc != 2)
  {
    std::cerr << "Usage: ./playback test.db\n";
    return 1;
  }

  std::cout << "Playback proof-of-concept\n";

  // Open the database
  raii_sqlite3::Database raiidb(argv[1], SQLITE_OPEN_READONLY);
  if (!raiidb)
  {
    std::cerr << "Failed to open database\n";
    return 1;
  }

  // Check if the schema version is supported
  std::string schemaVersion = checkVersion(raiidb);
  if (schemaVersion != "0.1.0")
  {
    std::cerr << "Database schema version '" << schemaVersion << "' is unsuported by this tool\n";
    return 1;
  }

  // Determine what topics were recorded in the database
  std::vector<KnownTopic> topics;
  if (!getTopics(raiidb, topics))
  {
    std::cerr << "Failed to get topics\n";
    return 1;
  }

  std::cout << "Playbing back " << topics.size() << " topics\n";

  // Create publishers for all of the recorded topics
  ignition::transport::Node node;
  std::unordered_map<KnownTopic, ignition::transport::Node::Publisher> publishers;
  for (const KnownTopic &topic : topics)
  {
    std::cout << "Playing back " << topic.name << "[" << topic.type << "]\n";
    ignition::transport::Node::Publisher pub = node.Advertise(topic.name, topic.type);

    if (!pub)
    {
      std::cerr << "Failed to create publisher\n";
      return 1;
    }
    publishers[topic] = pub;
  }

  // Step through all messages that were recorded
  const char *get_messages = "SELECT topic_id, time_recv_sec, time_recv_nano, message FROM messages ORDER BY time_recv_sec, time_recv_nano ASC;";
  raii_sqlite3::Statement statement(raiidb, get_messages);
  if (!statement)
  {
    std::cerr << "Failed to compile statement\n";
    return 1;
  }

  Time lastMessageTime;
  int published_first_message = false;

  // Iterate through all the messages in the result
  while (SQLITE_ROW == sqlite3_step(statement.Handle()))
  {
    // Get data
    Time nextTime;
    std::size_t topic_id = sqlite3_column_int64(statement.Handle(), 0);
    nextTime.sec = sqlite3_column_int64(statement.Handle(), 1);
    nextTime.nano = sqlite3_column_int64(statement.Handle(), 2);
    const void *data = sqlite3_column_blob(statement.Handle(), 3);
    std::size_t numData = sqlite3_column_bytes(statement.Handle(), 3);

    if (numData == 0 || data == nullptr)
    {
      std::cerr << "Message has no data, skipping\n";
      continue;
    }

    // Incomplete type used to get complete type from publishers map
    KnownTopic key;
    key.id = topic_id;
    auto iter = publishers.find(key);
    if (iter == publishers.end())
    {
      std::cerr << "Bag has a message from an unknown publisher, skipping\n";
      continue;
    }

    // Need a raw bytes publisher to publish messages that aren't in the version
    // of ignition messages this executable is linked with
    const google::protobuf::Descriptor* descriptor =
      google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(iter->first.type);
    if (!descriptor)
    {
      std::cerr << "This message cannot be published until ignition transport has a raw_bytes publisher(1)\n";
      continue;
    }
    const google::protobuf::Message* prototype = NULL;
    prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);

    if (!prototype)
    {
      std::cerr << "This message cannot be published until ignition transport has a raw_bytes publisher(2)\n";
      continue;
    }

    // Publish the first message right away, all others delay
    if (published_first_message)
    {
      // TODO use steady_clock to track the time spent publishing the last message
      // and alter the delay accordingly
      std::this_thread::sleep_for(nextTime - lastMessageTime);
    }
    else
    {
      published_first_message = true;
    }

    // Actually publish the message
    google::protobuf::Message* payload = prototype->New();
    std::string strData(reinterpret_cast<const char *>(data), numData);
    payload->ParseFromString(strData);
    iter->second.Publish(*payload);
    lastMessageTime = nextTime;
    std::cout << "Published message\n";
  }

  return 0;
};
