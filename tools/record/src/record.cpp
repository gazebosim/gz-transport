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

#include <ignition/transport.hh>
#include <sqlite3.h>

#include "config.hh"


typedef struct
{
  time_t time_rx;
  std::string topic;
  std::string message;
} ReceivedMessage;


// Global RAM buffer for messages
std::vector<ReceivedMessage> g_message_buffer;
std::mutex g_buffer_mutex;


// Create a brand new database
sqlite3 * createDatabase(const char * path)
{
  sqlite3 *db = nullptr;
  int return_code;
  char *errMsg;

  return_code = sqlite3_open(path, &db);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to open database\n";
    sqlite3_close(db);
    return nullptr;
  }

  return_code = sqlite3_extended_result_codes(db, 1);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to turn on extended error codes\n";
    sqlite3_close(db);
    return nullptr;
  }

  // Assume the file didn't exist before and create a blank schema
  std::cout << "Schema file: " << SCHEMA_PATH "/schema/0.1.0.sql\n";
  std::string schema;
  std::ifstream fin(SCHEMA_PATH "/schema/0.1.0.sql", std::ifstream::in);
  if (!fin)
  {
    std::cerr << "Failed to open schema file\n";
    sqlite3_close(db);
    return nullptr;
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
    sqlite3_close(db);
    return nullptr;
  }

  // Apply the schema to the database
  return_code = sqlite3_exec(db, schema.c_str(), NULL, 0, &errMsg);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to initialize schema: " << errMsg << "\n";
    sqlite3_free(errMsg);
    sqlite3_close(db);
    return nullptr;
  }

  return db;
}


/// \brief Write all buffered messages to the database
bool writeToDatabase(sqlite3 *db)
{
  int return_code;
  char *errMsg;

  const char *begin_transaction = "BEGIN;";
  const char *insert_message = "INSERT OR ROLLBACK INTO messages (time_recv_utc, message, topic_id) SELECT ?001, ?002, id FROM topics WHERE name LIKE ?003 LIMIT 1;";
  const char *end_transaction = "END;";

  // Begin transaction
  return_code = sqlite3_exec(db, begin_transaction, NULL, 0, &errMsg);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to begin transaction" << return_code << "\n";
    return false;
  }

  // Compile the statements 
  sqlite3_stmt *prepared_statement;
  return_code = sqlite3_prepare_v2(db, insert_message, -1, &prepared_statement, NULL);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to compile statement: " << return_code << "\n";
    return false;
  }

  std::lock_guard<std::mutex> guard(g_buffer_mutex);
  for (auto msg : g_message_buffer)
  {
    // Bind parameters
    return_code = sqlite3_bind_int(prepared_statement, 1, msg.time_rx);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind time received: " << return_code << "\n";
      sqlite3_finalize(prepared_statement);
      return false;
    }
    return_code = sqlite3_bind_blob(prepared_statement, 2, msg.message.c_str(), msg.message.size(), nullptr);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind message data: " << return_code << "\n";
      sqlite3_finalize(prepared_statement);
      return false;
    }
    return_code = sqlite3_bind_text(prepared_statement, 3, msg.topic.c_str(), msg.topic.size(), nullptr);
    if (return_code != SQLITE_OK)
    {
      std::cerr << "Failed to bind topic name: " << return_code << "\n";
      sqlite3_finalize(prepared_statement);
      return false;
    }

    // Execute the statement
    return_code = sqlite3_step(prepared_statement);
    if (return_code != SQLITE_DONE)
    {
      std::cerr << "Unexpected return code while stepping(1): " << return_code << "\n";
      sqlite3_finalize(prepared_statement);
      return false;
    }

    // Reset for another round
    sqlite3_reset(prepared_statement);
  }

  // End transaction
  return_code = sqlite3_exec(db, end_transaction, NULL, 0, &errMsg);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to end transaction" << return_code << "\n";
    sqlite3_finalize(prepared_statement);
    return false;
  }

  sqlite3_finalize(prepared_statement);
  return true;
}

// Add a topic and message type into the database
bool insertTopic(sqlite3 *db, std::string topic, std::string msg_type)
{
  int return_code;
  char *errMsg;

  const char *begin_transaction = "BEGIN;";
  const char *insert_message_type = "INSERT OR IGNORE INTO message_types (name) VALUES (?001);";
  const char *insert_topic = "INSERT OR ROLLBACK INTO topics (name, message_type_id) SELECT ?002, id FROM message_types WHERE name LIKE ?001 LIMIT 1;";
  const char *end_transaction = "END;";

  // Begin transaction
  return_code = sqlite3_exec(db, begin_transaction, NULL, 0, &errMsg);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to begin transaction" << return_code << "\n";
    return false;
  }

  // Compile the statements 
  sqlite3_stmt *message_type_statement;
  return_code = sqlite3_prepare_v2(db, insert_message_type, -1, &message_type_statement, NULL);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to compile statement(1): " << return_code << "\n";
    return false;
  }
  sqlite3_stmt *topic_statement;
  return_code = sqlite3_prepare_v2(db, insert_topic, -1, &topic_statement, NULL);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to compile statement(2): " << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    return false;
  }

  // Bind parameters
  return_code = sqlite3_bind_text(message_type_statement, 1, msg_type.c_str(), msg_type.size(), nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to bind message type name(1): " << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    sqlite3_finalize(topic_statement);
    return false;
  }
  return_code = sqlite3_bind_text(topic_statement, 1, msg_type.c_str(), msg_type.size(), nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to bind message type name(2): " << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    sqlite3_finalize(topic_statement);
    return false;
  }
  return_code = sqlite3_bind_text(topic_statement, 2, topic.c_str(), topic.size(), nullptr);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to bind topic name: " << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    sqlite3_finalize(topic_statement);
    return false;
  }

  // Execute the statements
  return_code = sqlite3_step(message_type_statement);
  if (return_code != SQLITE_DONE)
  {
    std::cerr << "Unexpected return code while stepping(1): " << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    sqlite3_finalize(topic_statement);
    return false;
  }
  return_code = sqlite3_step(topic_statement);
  if (return_code != SQLITE_DONE)
  {
    std::cerr << "Unexpected return code while stepping(2): " << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    sqlite3_finalize(topic_statement);
    return false;
  }

  // End transaction
  return_code = sqlite3_exec(db, end_transaction, NULL, 0, &errMsg);
  if (return_code != SQLITE_OK)
  {
    std::cerr << "Failed to end transaction" << return_code << "\n";
    sqlite3_finalize(message_type_statement);
    sqlite3_finalize(topic_statement);
    return false;
  }

  sqlite3_finalize(message_type_statement);
  sqlite3_finalize(topic_statement);
  return true;
}


/// \brief Function called each time a topic update is received.
void onMessageReceived(
    const google::protobuf::Message &_msg,
    const ignition::transport::MessageInfo &_info)
{
  ReceivedMessage m;
  m.time_rx = time(NULL);
  m.topic = _info.Topic();
  _msg.SerializeToString(&(m.message));
  std::cout << "Received message on " << m.topic << "\n";
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
  sqlite3 *db = createDatabase(argv[1]);
  if (nullptr == db)
  {
    return 1;
  }

  // Set up topics to be recorded
  for (auto topic : all_topics)
  {
    std::cout << "Recording " << topic << "\n";

    // Insert a row for the message in the database
    // TODO how to determine type of a published topic?
    if (!insertTopic(db, topic, ".unknown.message.type"))
    {
      return 1;
    }

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
    if (!writeToDatabase(db))
    {
      sqlite3_close(db);
      return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "Finished recording\n";

  // Sure would be nice to have a destructor
  sqlite3_close(db);
  return 0;
}
