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
#include <fstream>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include <ignition/transport.hh>
#include <sqlite3.h>

#include "config.hh"
#include "src/raii-sqlite3.hh"

typedef struct
{
  std::string topic;
  std::string messageType;
} KnownTopic;


typedef struct
{
  int64_t time_rx_sec;
  int64_t time_rx_nano;
  KnownTopic topic;
  std::string message;
} ReceivedMessage;


/// \brief topics that have been added to the database
std::vector<KnownTopic> g_knownTopics;
/// \brief topics that have not yet been added to the database
std::vector<KnownTopic> g_newTopics;
std::mutex g_topic_mutex;


/// \brief Return true if two KnownTopic's are identical
bool operator==(const KnownTopic &_lhs, const KnownTopic &_rhs)
{
  return _lhs.topic == _rhs.topic && _lhs.messageType == _rhs.messageType;
}


bool insertTopic(raii_sqlite3::Database &db, std::string topic, std::string msg_type);


// Global RAM buffer for messages
std::vector<ReceivedMessage> g_message_buffer;
std::mutex g_buffer_mutex;

// Variable when added to steady_clock gives UTC time in nanoseconds
std::chrono::nanoseconds g_wallMinusMonoNS;


// Create a brand new database
bool initDatabase(raii_sqlite3::Database &db)
{
  int return_code;

  // Assume the file didn't exist before and create a blank schema
  std::cout << "Schema file: " << SCHEMA_PATH "/schema/0.1.0.sql\n";
  std::string schema;
  std::ifstream fin(SCHEMA_PATH "/schema/0.1.0.sql", std::ifstream::in);
  if (!fin)
  {
    std::cerr << "Failed to open schema file\n";
    return false;
  }

  // get length of file:
  fin.seekg (0, fin.end);
  int length = fin.tellg();
  fin.seekg (0, fin.beg);

  // Try to read all of file at once
  char *buffer = new char [length];
  fin.read(buffer, length);
  schema = buffer;
  delete buffer;
  if (!fin)
  {
    std::cerr << "Failed to read file in one go\n";
    return false;
  }

  // Apply the schema to the database
  return_code = sqlite3_exec(db.Handle(), schema.c_str(), NULL, 0, NULL);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to initialize schema: " << sqlite3_errmsg(db.Handle()) << "\n";
    return false;
  }

  return true;
}


/// \brief Write all buffered messages to the database
bool writeToDatabase(raii_sqlite3::Database &db)
{
  int return_code;

  const char *begin_transaction = "BEGIN;";
  // TODO(sloretz) cache the topic id rather than quering the topic ID on every message insert;
  const char *insert_message = "INSERT OR ROLLBACK INTO messages (time_recv_sec, time_recv_nano, message, topic_id)"
    " SELECT ?001, ?002, ?003, topics.id FROM topics JOIN message_types ON topics.message_type_id = message_types.id"
    " WHERE topics.name = ?004 AND message_types.name = ?005 LIMIT 1;";
  const char *end_transaction = "END;";

  // Begin transaction
  return_code = sqlite3_exec(db.Handle(), begin_transaction, NULL, 0, nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to begin transaction" << sqlite3_errmsg(db.Handle()) << "\n";
    return false;
  }

  // Insert topics and messages types
  {
    std::lock_guard<std::mutex> guard(g_topic_mutex);
    for (auto topic : g_newTopics)
    {
      if (!insertTopic(db, topic.topic, topic.messageType))
      {
        std::cerr << "Failed to insert topic\n";
        return false;
      }
    }
    g_knownTopics.insert(g_knownTopics.end(), g_newTopics.begin(), g_newTopics.end());
    g_newTopics.clear();
  }

  // Insert messages
  // Compile the statements 
  raii_sqlite3::Statement statement(db, insert_message);
  if (!statement)
  {
    std::cerr << "Failed to compile statement\n";
    return false;
  }

  std::lock_guard<std::mutex> guard(g_buffer_mutex);
  for (auto msg : g_message_buffer)
  {
    // Bind parameters
    return_code = sqlite3_bind_int(statement.Handle(), 1, msg.time_rx_sec);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind time received(s): " << return_code << "\n";
      return false;
    }
    return_code = sqlite3_bind_int(statement.Handle(), 2, msg.time_rx_nano);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind time received(ns): " << return_code << "\n";
      return false;
    }
    return_code = sqlite3_bind_blob(statement.Handle(), 3, msg.message.c_str(), msg.message.size(), nullptr);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind message data: " << return_code << "\n";
      return false;
    }
    return_code = sqlite3_bind_text(statement.Handle(), 4, msg.topic.topic.c_str(), msg.topic.topic.size(), nullptr);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind topic name: " << return_code << "\n";
      return false;
    }
    return_code = sqlite3_bind_text(statement.Handle(), 5, msg.topic.messageType.c_str(), msg.topic.messageType.size(), nullptr);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind message type name: " << return_code << "\n";
      return false;
    }

    // Execute the statement
    return_code = sqlite3_step(statement.Handle());
    if (return_code != SQLITE_DONE)
    {
      std::cerr << "Unexpected return code while stepping(1): " << return_code << "\n";
      return false;
    }

    // Reset for another round
    sqlite3_reset(statement.Handle());
  }
  g_message_buffer.clear();

  // End transaction
  return_code = sqlite3_exec(db.Handle(), end_transaction, NULL, 0, nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to end transaction" << return_code << "\n";
    return false;
  }

  return true;
}

// Add a topic and message type into the database
bool insertTopic(raii_sqlite3::Database &db, std::string topic, std::string msg_type)
{
  int return_code;

  const char *insert_message_type = "INSERT OR IGNORE INTO message_types (name) VALUES (?001);";
  const char *insert_topic = "INSERT OR IGNORE INTO topics (name, message_type_id) SELECT ?002, id FROM message_types WHERE name = ?001 LIMIT 1;";

  // Compile the statements 
  raii_sqlite3::Statement message_type_statement(db, insert_message_type);
  if (!message_type_statement)
  {
    std::cerr << "Failed to compile statement(1)\n";
    return false;
  }
  raii_sqlite3::Statement topic_statement(db, insert_topic);
  if (!topic_statement)
  {
    std::cerr << "Failed to compile statement(2)\n";
    return false;
  }

  // Bind parameters
  return_code = sqlite3_bind_text(message_type_statement.Handle(), 1, msg_type.c_str(), msg_type.size(), nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to bind message type name(1): " << return_code << "\n";
    return false;
  }
  return_code = sqlite3_bind_text(topic_statement.Handle(), 1, msg_type.c_str(), msg_type.size(), nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to bind message type name(2): " << return_code << "\n";
    return false;
  }
  return_code = sqlite3_bind_text(topic_statement.Handle(), 2, topic.c_str(), topic.size(), nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to bind topic name: " << return_code << "\n";
    return false;
  }

  // Execute the statements
  return_code = sqlite3_step(message_type_statement.Handle());
  if (return_code != SQLITE_DONE)
  {
    std::cerr << "Unexpected return code while stepping(1): " << return_code << "\n";
    return false;
  }
  return_code = sqlite3_step(topic_statement.Handle());
  if (return_code != SQLITE_DONE)
  {
    std::cerr << "Unexpected return code while stepping(2): " << return_code << "\n";
    return false;
  }
  return true;
}


/// \brief Function called each time a topic update is received.
void onMessageReceived(
    const google::protobuf::Message &_msg,
    const ignition::transport::MessageInfo &_info)
{
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::nanoseconds nowNS(now.time_since_epoch());
  std::chrono::nanoseconds utcNS = g_wallMinusMonoNS + nowNS;
  std::chrono::seconds utcS = std::chrono::duration_cast<std::chrono::seconds>(
      utcNS);

  KnownTopic newTopic;
  newTopic.topic = _info.Topic();
  newTopic.messageType = _info.Type();

  ReceivedMessage m;
  m.time_rx_sec = utcS.count();
  m.time_rx_nano = (utcNS - std::chrono::nanoseconds(utcS)).count();
  m.topic = newTopic;
  _msg.SerializeToString(&(m.message));
  std::cout << "Received message on " << newTopic.topic << " type " << newTopic.messageType << "\n";

  {
    std::lock_guard<std::mutex> guard(g_topic_mutex);
    if(g_knownTopics.end() == std::find(g_knownTopics.begin(), g_knownTopics.end(), newTopic))
    {
      if(g_newTopics.end() == std::find(g_newTopics.begin(), g_newTopics.end(), newTopic))
      {
        std::cout << "New topic " << newTopic.topic << " type " << newTopic.messageType << "\n";
        g_newTopics.push_back(newTopic);
      }
    }
  }

  std::lock_guard<std::mutex> guard(g_buffer_mutex);
  g_message_buffer.push_back(m);
}


int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: ./record test.db\n";
    return 1;
  }

  // Record the start time to sync system and steady clock
  // Use std::time() since on most systems it is UTC
  // Not sure if std::chrono::system_clock::now() is UTC on osx/windows
  // https://stackoverflow.com/questions/14504870
  std::chrono::seconds wallStart = std::chrono::seconds(std::time(NULL));
  // Monotonic clock
  std::chrono::steady_clock::time_point monoStart
    = std::chrono::steady_clock::now();

  // Get a value to sync the two clocks
  std::chrono::nanoseconds wallStartNS(wallStart);
  std::chrono::nanoseconds monoStartNS(monoStart.time_since_epoch());
  g_wallMinusMonoNS = wallStartNS - monoStartNS;

  std::cout << "Record proof-of-concept\n";
  ignition::transport::Node node;

  std::vector<std::string> all_topics;
  node.TopicList(all_topics);

  if (all_topics.empty())
  {
    std::cerr << "No topics to record\n";
    return 1;
  }

  std::cout << "Recording " << all_topics.size() << " topics\n";

  // Assume database doesn't exist already and create it
  raii_sqlite3::Database raiidb(argv[1], SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  if (!raiidb)
  {
    std::cerr << "Failed to open database\n";
    return 1;
  }

  if (!initDatabase(raiidb))
  {
    std::cerr << "Failed to init database\n";
    return 1;
  }

  // Set up topics to be recorded
  for (auto topic : all_topics)
  {
    std::cout << "Recording " << topic << "\n";
    if (!node.Subscribe(topic, onMessageReceived))
    {
      std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
      return 1;
    }
  }

  // Write to database in main thread
  std::cout << "Recording for 30 seconds\n";
  for (int i = 0; i < 30; ++i)
  {
    if (!writeToDatabase(raiidb))
    {
      return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "Finished recording\n";

  return 0;
}
