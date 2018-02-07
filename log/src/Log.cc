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
#include <cstdlib>
#include <functional>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "Console.hh"

#include "ignition/transport/log/Log.hh"
#include "src/raii-sqlite3.hh"
#include "build_config.hh"

#include "Descriptor.hh"

using namespace ignition::transport;
using namespace ignition::transport::log;


/// \brief Nanoseconds Per Second
const sqlite3_int64 NS_PER_SEC = 1000000000;

/// \brief Private implementation
class ignition::transport::log::Log::Implementation
{
  /// \internal \sa Log::Descriptor()
  public: const log::Descriptor *Descriptor() const;

  /// \brief End transaction if enough time has passed since it began
  /// \return false if there is a sqlite error
  public: bool EndTransactionIfEnoughTimeHasPassed();

  /// \brief Begin transaction if one isn't already open
  /// \return false if there is a sqlite error
  public: bool BeginTransactionIfNotInOne();

  /// \brief Get topic_id associated with a topic name and message type
  /// If the topic is not in the log it will be added
  /// \note Invalidates the descriptor if a topic is inserted
  /// \param[in] _name the name of the topic
  /// \param[in] _type the name of the message type
  /// \return topic_id or -1 if one could not be produced
  public: int64_t InsertOrGetTopicId(
      const std::string &_name, const std::string &_type);

  /// \brief Insert a message into the database
  public: bool InsertMessage(const std::chrono::nanoseconds &_time,
      int64_t _topic, const void *_data, std::size_t _len);

  /// \brief Return true if enough time has passed since the last transaction
  /// \return true if the transaction has lasted long enough
  public: bool TimeForNewTransaction() const;

  /// \brief SQLite3 database pointer wrapper
  public: std::shared_ptr<raii_sqlite3::Database> db;

  /// \brief True if a transaction is in progress
  public: bool inTransaction = false;

  /// \brief Maps topic name/type pairs to an id in the topics table
  public: TopicKeyMap topics;

  /// \brief last time the transaction was ended
  public: std::chrono::steady_clock::time_point lastTransaction;

  /// \brief duration between transactions
  public: std::chrono::milliseconds transactionPeriod;

  /// \brief Flag to track whether we need to generate a new Descriptor
  private: mutable bool needNewDescriptor = true;

  /// \brief Descriptor which provides insight to the column IDs in the database
  public: mutable log::Descriptor descriptor;
};

//////////////////////////////////////////////////
const log::Descriptor *Log::Implementation::Descriptor() const
{
  if (!this->db)
    return nullptr;

  if (this->needNewDescriptor)
  {
    TopicKeyMap topicsInLog;

    // Prepare the statement
    const char * sql_statement =
      "SELECT topics.id, topics.name, message_types.name FROM topics"
      " JOIN message_types ON topics.message_type_id = message_types.id;";

    raii_sqlite3::Statement topic_ids_statement(*(this->db), sql_statement);
    if (!topic_ids_statement)
    {
      LERR("Failed to compile statement to get topic ids\n");
      return nullptr;
    }

    // Execute the statement
    int returnCode;
    do
    {
      returnCode = sqlite3_step(topic_ids_statement.Handle());
      if (returnCode == SQLITE_ROW)
      {
        // get the data about the topic
        sqlite_int64 topicId = sqlite3_column_int64(
            topic_ids_statement.Handle(), 0);

        const unsigned char *topicName = sqlite3_column_text(
            topic_ids_statement.Handle(), 1);
        std::size_t lenTopicName = sqlite3_column_bytes(
            topic_ids_statement.Handle(), 1);

        const unsigned char *typeName = sqlite3_column_text(
            topic_ids_statement.Handle(), 2);
        std::size_t lenTypeName = sqlite3_column_bytes(
            topic_ids_statement.Handle(), 2);

        TopicKey key;
        key.topic = std::string(
            reinterpret_cast<const char *>(topicName), lenTopicName);
        key.type = std::string(
            reinterpret_cast<const char *>(typeName), lenTypeName);
        topicsInLog[key] = topicId;
        LDBG(key.topic << "|" << key.type << "|" << topicId << "\n");
      }
      else if (returnCode != SQLITE_DONE)
      {
        LERR("Failed query topic ids: " << sqlite3_errmsg(
            this->db->Handle()) << "\n");
        return nullptr;
      }
    } while (returnCode == SQLITE_ROW);

    // Save the result into the descriptor
    this->needNewDescriptor = false;
    descriptor.dataPtr->Reset(topicsInLog);
  }

  return &this->descriptor;
}

//////////////////////////////////////////////////
bool Log::Implementation::EndTransactionIfEnoughTimeHasPassed()
{
  if (!this->TimeForNewTransaction())
  {
    return true;
  }

  // End the transaction
  int returnCode = sqlite3_exec(
      this->db->Handle(), "END;", NULL, 0, nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to end transaction" << returnCode << "\n");
    return false;
  }
  LDBG("Ended transaction\n");
  this->inTransaction = false;
  return true;
}

//////////////////////////////////////////////////
bool Log::Implementation::BeginTransactionIfNotInOne()
{
  if (this->inTransaction)
    return true;

  int returnCode = sqlite3_exec(
      this->db->Handle(), "BEGIN;", NULL, 0, nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to begin transaction" << returnCode << "\n");
    return false;
  }
  this->inTransaction = true;
  LDBG("Began transaction\n");
  this->lastTransaction = std::chrono::steady_clock::now();
  return true;
}

//////////////////////////////////////////////////
bool Log::Implementation::TimeForNewTransaction() const
{
  auto now = std::chrono::steady_clock::now();
  return now - this->transactionPeriod > this->lastTransaction;
}

//////////////////////////////////////////////////
int64_t Log::Implementation::InsertOrGetTopicId(
    const std::string &_name,
    const std::string &_type)
{
  // If the name and type is known, return a cached ID
  // Call method to get side effect of updating descriptor
  const log::Descriptor *desc = this->Descriptor();
  if (nullptr == desc)
  {
    return -1;
  }

  int64_t topicId = desc->TopicId(_name, _type);
  if (topicId >= 0)
  {
    return topicId;
  }

  // Inserting a new topic invalidates the descriptor
  this->needNewDescriptor = true;

  // Otherwise insert it into the database and return the new topic_id
  const std::string sql_message_type =
    "INSERT OR IGNORE INTO message_types (name) VALUES (?001);";
  const std::string sql_topic =
    "INSERT INTO topics (name, message_type_id)"
    " SELECT ?002, id FROM message_types WHERE name = ?001 LIMIT 1;";

  raii_sqlite3::Statement message_type_statement(
      *(this->db), sql_message_type);
  if (!message_type_statement)
  {
    LERR("Failed to compile statement to insert message type\n");
    return -1;
  }
  raii_sqlite3::Statement topic_statement(
      *(this->db), sql_topic);
  if (!topic_statement)
  {
    LERR("Failed to compile statement to insert topic\n");
    return -1;
  }

  int returnCode;
  // Bind parameters
  returnCode = sqlite3_bind_text(
      message_type_statement.Handle(), 1, _type.c_str(), _type.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind message type name(1): " << returnCode << "\n");
    return -1;
  }
  returnCode = sqlite3_bind_text(
      topic_statement.Handle(), 1, _type.c_str(), _type.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind message type name(2): " << returnCode << "\n");
    return -1;
  }
  returnCode = sqlite3_bind_text(
      topic_statement.Handle(), 2, _name.c_str(), _name.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind topic name: " << returnCode << "\n");
    return -1;
  }

  // Execute the statements
  returnCode = sqlite3_step(message_type_statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    LERR("Failed to insert message type: " << returnCode << "\n");
    return -1;
  }
  returnCode = sqlite3_step(topic_statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    LERR("Faild to insert topic: " << returnCode << "\n");
    return -1;
  }

  // topics.id is an alias for rowid
  int64_t id = sqlite3_last_insert_rowid(this->db->Handle());
  LDBG("Inserted '" << _name << "'[" << _type << "]\n");
  return id;
}

//////////////////////////////////////////////////
bool Log::Implementation::InsertMessage(
    const std::chrono::nanoseconds &_time,
    int64_t _topic,
    const void *_data,
    std::size_t _len)
{
  int returnCode;
  const std::string sql_message =
    "INSERT INTO messages (time_recv, message, topic_id)"
    "VALUES (?001, ?002, ?003);";

  // Compile the statement
  raii_sqlite3::Statement statement(*(this->db), sql_message);
  if (!statement)
  {
    LERR("Failed to compile insert message statement\n");
    return false;
  }

  // Bind parameters
  returnCode = sqlite3_bind_int64(statement.Handle(), 1, _time.count());
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind time received: " << returnCode << "\n");
    return false;
  }
  returnCode = sqlite3_bind_blob(statement.Handle(), 2, _data, _len, nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind message data: " << returnCode << "\n");
    return false;
  }
  returnCode = sqlite3_bind_int(statement.Handle(), 3, _topic);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind topic_id: " << returnCode << "\n");
    return false;
  }

  // Execute the statement
  returnCode = sqlite3_step(statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    LERR("Failed to insert message: " << returnCode << "\n");
    return false;
  }
  return true;
}

//////////////////////////////////////////////////
Log::Log()
  : dataPtr(new Implementation)
{
  // Default to 2 transactions per second
  this->dataPtr->transactionPeriod = std::chrono::milliseconds(500);
}

//////////////////////////////////////////////////
Log::Log(Log &&_other)  // NOLINT
  : dataPtr(std::move(_other.dataPtr))
{
}

//////////////////////////////////////////////////
Log::~Log()
{
  if (this->dataPtr && this->dataPtr->inTransaction)
  {
    this->dataPtr->EndTransactionIfEnoughTimeHasPassed();
  }
}

//////////////////////////////////////////////////
bool Log::Valid() const
{
  return this->dataPtr && this->dataPtr->db && *(this->dataPtr->db);
}

//////////////////////////////////////////////////
bool Log::Open(const std::string &_file, std::ios_base::openmode _mode)
{
  // Open the SQLite3 database
  if (this->dataPtr->db)
  {
    LERR("A database is already open\n");
    return false;
  }
  int64_t modeSQL = 0;
  if (std::ios_base::out & _mode)
  {
    modeSQL = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  }
  else if (std::ios_base::in & _mode)
  {
    modeSQL = SQLITE_OPEN_READONLY;
  }

  std::unique_ptr<raii_sqlite3::Database> db(
      new raii_sqlite3::Database(_file, modeSQL));
  if (!*(db))
  {
    LERR("Failed to open sqlite3 database\n");
    return false;
  }

  // Don't need to create a schema if this is read only
  if (modeSQL != SQLITE_OPEN_READONLY)
  {
    // Test hook so tests can be run before `make install`
    std::string schemaFile;
    const char *envPath = std::getenv(SchemaLocationEnvVar.c_str());
    if (envPath)
    {
      schemaFile = envPath;
    }
    else
    {
      schemaFile = SCHEMA_INSTALL_PATH;
    }
    schemaFile += "/0.1.0.sql";

    // Assume the database is uninitialized; use the schema to initialize it
    LDBG("Schema file: " << schemaFile << "\n");
    std::ifstream fin(schemaFile, std::ifstream::in);
    if (!fin)
    {
      LERR("Failed to open schema [" << schemaFile << "].\n"
          << " Set " << SchemaLocationEnvVar << " to the schema location.\n");
      return false;
    }

    // Read the schema file
    std::string schema;
    char buffer[4096];
    while (fin)
    {
      fin.read(buffer, sizeof(buffer));
      schema.insert(schema.size(), buffer, fin.gcount());
    }
    if (schema.empty())
    {
      LERR("Failed to read schema file [" << schemaFile << "]\n");
      return false;
    }

    // Apply the schema to the database
    int returnCode = sqlite3_exec(db->Handle(), schema.c_str(), NULL, 0, NULL);
    if (returnCode != SQLITE_OK)
    {
      LERR("Failed to open log: " << sqlite3_errmsg(db->Handle()) << "\n");
      return false;
    }
  }

  this->dataPtr->db = std::move(db);

  // Check the schema version
  // TODO(sloretz) handle multiple versions
  std::string version = this->Version();
  if ("0.1.0" != this->Version())
  {
    LERR("Log file Version '" << version << "' is unsupported by this tool\n");
    this->dataPtr->db.reset();
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
const log::Descriptor *Log::Descriptor() const
{
  return this->dataPtr->Descriptor();
}

//////////////////////////////////////////////////
bool Log::InsertMessage(
    const std::chrono::nanoseconds &_time,
    const std::string &_topic, const std::string &_type,
    const void *_data, std::size_t _len)
{
  if (!this->Valid())
  {
    return false;
  }

  // Need to insert multiple messages pertransaction for best performance
  if (!this->dataPtr->BeginTransactionIfNotInOne())
  {
    return false;
  }

  // Get the topics.id for this name and message type
  int64_t topicId = this->dataPtr->InsertOrGetTopicId(_topic, _type);
  if (topicId < 0)
  {
    return false;
  }

  // Insert the message into the database
  if (!this->dataPtr->InsertMessage(_time, topicId, _data, _len))
  {
    return false;
  }

  // Finish the transaction if enough time has passed
  if (!this->dataPtr->EndTransactionIfEnoughTimeHasPassed())
  {
    // Something is really busted if this happens
    LERR("Failed to end transcation: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n");
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
std::string Log::Version()
{
  if (!this->Valid())
  {
    return "";
  }

  // Compile the statement
  const char *get_version =
    "SELECT to_version FROM migrations ORDER BY id DESC LIMIT 1;";
  raii_sqlite3::Statement statement(*(this->dataPtr->db), get_version);
  if (!statement)
  {
    LERR("Failed to compile version query statement\n");
    return "";
  }

  // Try to run it
  int result_code = sqlite3_step(statement.Handle());
  if (result_code != SQLITE_ROW)
  {
    LERR("Database has no version\n");
    return "";
  }

  // Version is free'd automatically when statement is destructed
  const unsigned char *version = sqlite3_column_text(statement.Handle(), 0);
  return std::string(reinterpret_cast<const char *>(version));
}
