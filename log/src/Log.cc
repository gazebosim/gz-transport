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
#include <unordered_map>
#include <utility>

#include <ignition/common/Console.hh>

#include "ignition/transport/log/Log.hh"
#include "src/MsgIterPrivate.hh"
#include "src/raii-sqlite3.hh"
#include "build_config.hh"


using namespace ignition::transport;
using namespace ignition::transport::log;


/// \brief Nanoseconds Per Second
const sqlite3_int64 NS_PER_SEC = 1000000000;


//////////////////////////////////////////////////
/// \brief allow pair of strings to be a key in a map
namespace std {
  template <> struct hash<std::pair<std::string, std::string>>
  {
    size_t operator()(const std::pair<std::string, std::string> &_topic) const
    {
      // Terrible, gets the job done
      return (std::hash<std::string>()(_topic.first) << 16)
        + std::hash<std::string>()(_topic.second);
    }
  };
}

/// \brief Private implementation
class ignition::transport::log::LogPrivate
{
  /// \brief End transaction if enough time has passed since it began
  /// \return false if there is a sqlite error
  public: bool EndTransactionIfEnoughTimeHasPassed();

  /// \brief Begin transaction if one isn't already open
  /// \return false if there is a sqlite error
  public: bool BeginTransactionIfNotInOne();

  /// \brief Get topic_id associated with a topic name and message type
  /// \param[in] _name the name of the topic
  /// \param[in] _type the name of the message type
  /// \return topic_id or -1 if one could not be produced
  public: int64_t TopicId(const std::string &_name, const std::string &_type);

  /// \brief Insert a message into the database
  public: bool InsertMessage(const common::Time &_time, int64_t _topic,
      const void *_data, std::size_t _len);

  /// \brief Return true if enough time has passed since the last transaction
  /// \return true if the transaction has lasted long enough
  public: bool TimeForNewTransaction() const;

  /// \brief SQLite3 database pointer wrapper
  public: std::unique_ptr<raii_sqlite3::Database> db;

  /// \brief True if a transaction is in progress
  public: bool inTransaction = false;

  /// \brief Maps topic name/type pairs to an id in the topics table
  public:
    std::unordered_map<std::pair<std::string, std::string>, int64_t> topics;

  /// \brief last time the transaction was ended
  public: std::chrono::steady_clock::time_point lastTransaction;

  /// \brief duration between transactions
  public: std::chrono::milliseconds transactionPeriod;
};

//////////////////////////////////////////////////
bool LogPrivate::EndTransactionIfEnoughTimeHasPassed()
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
    ignerr << "Failed to end transaction" << returnCode << "\n";
    return false;
  }
  igndbg << "Ended transaction\n";
  this->inTransaction = false;
  return true;
}

//////////////////////////////////////////////////
bool LogPrivate::BeginTransactionIfNotInOne()
{
  if (this->inTransaction)
    return true;

  int returnCode = sqlite3_exec(
      this->db->Handle(), "BEGIN;", NULL, 0, nullptr);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to begin transaction" << returnCode << "\n";
    return false;
  }
  this->inTransaction = true;
  igndbg << "Began transaction\n";
  this->lastTransaction = std::chrono::steady_clock::now();
  return true;
}

//////////////////////////////////////////////////
bool LogPrivate::TimeForNewTransaction() const
{
  auto now = std::chrono::steady_clock::now();
  return now - this->transactionPeriod > this->lastTransaction;
}

//////////////////////////////////////////////////
int64_t LogPrivate::TopicId(const std::string &_name, const std::string &_type)
{
  int returnCode;
  // If the name and type is known, return a cached ID
  auto key = std::make_pair(_name, _type);
  auto topicIter = this->topics.find(key);
  if (topicIter != this->topics.end())
  {
    return topicIter->second;
  }

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
    ignerr << "Failed to compile statement to insert message type\n";
    return -1;
  }
  raii_sqlite3::Statement topic_statement(
      *(this->db), sql_topic);
  if (!topic_statement)
  {
    ignerr << "Failed to compile statement to insert topic\n";
    return -1;
  }

  // Bind parameters
  returnCode = sqlite3_bind_text(
      message_type_statement.Handle(), 1, _type.c_str(), _type.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to bind message type name(1): " << returnCode << "\n";
    return -1;
  }
  returnCode = sqlite3_bind_text(
      topic_statement.Handle(), 1, _type.c_str(), _type.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to bind message type name(2): " << returnCode << "\n";
    return -1;
  }
  returnCode = sqlite3_bind_text(
      topic_statement.Handle(), 2, _name.c_str(), _name.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to bind topic name: " << returnCode << "\n";
    return -1;
  }

  // Execute the statements
  returnCode = sqlite3_step(message_type_statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    ignerr << "Failed to insert message type: " << returnCode << "\n";
    return -1;
  }
  returnCode = sqlite3_step(topic_statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    ignerr << "Faild to insert topic: " << returnCode << "\n";
    return -1;
  }

  // topics.id is an alias for rowid
  int64_t id = sqlite3_last_insert_rowid(this->db->Handle());
  this->topics[key] = id;
  igndbg << "Inserted '" << _name << "'[" << _type << "]\n";
  return id;
}

//////////////////////////////////////////////////
bool LogPrivate::InsertMessage(const common::Time &_time, int64_t _topic,
      const void *_data, std::size_t _len)
{
  int returnCode;
  const std::string sql_message =
    "INSERT INTO messages (time_recv, message, topic_id)"
    "VALUES (?001, ?002, ?003);";

  // Compile the statement
  raii_sqlite3::Statement statement(*(this->db), sql_message);
  if (!statement)
  {
    ignerr << "Failed to compile insert message statement\n";
    return false;
  }

  // Bind parameters
  returnCode = sqlite3_bind_int64(statement.Handle(), 1,
      _time.sec * NS_PER_SEC + _time.nsec);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to bind time received: " << returnCode << "\n";
    return false;
  }
  returnCode = sqlite3_bind_blob(statement.Handle(), 2, _data, _len, nullptr);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to bind message data: " << returnCode << "\n";
    return false;
  }
  returnCode = sqlite3_bind_int(statement.Handle(), 3, _topic);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to bind topic_id: " << returnCode << "\n";
    return false;
  }

  // Execute the statement
  returnCode = sqlite3_step(statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    ignerr << "Failed to insert message: " << returnCode << "\n";
    return false;
  }
  return true;
}

//////////////////////////////////////////////////
Log::Log()
  : dataPtr(new LogPrivate)
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
  if (this->dataPtr->inTransaction)
  {
    this->dataPtr->EndTransactionIfEnoughTimeHasPassed();
  }
}

//////////////////////////////////////////////////
bool Log::Valid() const
{
  return this->dataPtr->db.operator bool();
}

//////////////////////////////////////////////////
bool Log::Open(const std::string &_file, std::ios_base::openmode _mode)
{
  int returnCode;

  // Open the SQLite3 database
  if (this->dataPtr->db)
  {
    ignerr << "A database is already open\n";
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

  this->dataPtr->db.reset(new raii_sqlite3::Database(_file, modeSQL));
  if (!*(this->dataPtr->db))
  {
    ignerr << "Failed to open sqlite3 database\n";
    return false;
  }

  // Don't need to create a schema if this is read only
  if (modeSQL == SQLITE_OPEN_READONLY)
    return true;

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
  igndbg << "Schema file: " << schemaFile << "\n";
  std::ifstream fin(schemaFile, std::ifstream::in);
  if (!fin)
  {
    ignerr << "Failed to open schema [" << schemaFile << "].\n"
           << " -- If the schema file is missing, then either:\n"
           << " 1. (Re)install ignition-transport-log or\n"
           << " 2. Set the " << SchemaLocationEnvVar << " environment variable "
           << "to the correct schema location.\n\n";
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
    ignerr << "Failed to read schema file [" << schemaFile << "]\n";
    return false;
  }

  // Apply the schema to the database
  returnCode = sqlite3_exec(
      this->dataPtr->db->Handle(), schema.c_str(), NULL, 0, NULL);
  if (returnCode != SQLITE_OK)
  {
    ignerr << "Failed to create log: " << sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
bool Log::InsertMessage(
    const common::Time &_time,
    const std::string &_topic, const std::string &_type,
    const void *_data, std::size_t _len)
{
  // Need to insert multiple messages pertransaction for best performance
  if (!this->dataPtr->BeginTransactionIfNotInOne())
  {
    return false;
  }

  // Get the topics.id for this name and message type
  int64_t topicId = this->dataPtr->TopicId(_topic, _type);
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
    ignerr << "Failed to end transcation: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
MsgIter Log::AllMessages()
{
  const char *sql = "SELECT messages.id, messages.time_recv, topics.name,"
    " message_types.name, messages.message FROM messages JOIN topics ON"
    " topics.id = messages.topic_id JOIN message_types ON"
    " message_types.id = topics.message_type_id ORDER BY messages.time_recv;";
  std::unique_ptr<raii_sqlite3::Statement> statement(
      new raii_sqlite3::Statement(*(this->dataPtr->db), sql));
  if (!*statement)
  {
    ignerr << "Failed to query messages: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
    return MsgIter();
  }
  std::unique_ptr<MsgIterPrivate> msgPriv(new MsgIterPrivate);
  msgPriv->statement = std::move(statement);
  return MsgIter(std::move(msgPriv));
}

//////////////////////////////////////////////////
MsgIter Log::QueryMessages(const std::unordered_set<std::string> &_topics)
{
  if (_topics.empty())
  {
    ignwarn << "No topics given\n";
    return MsgIter();
  }

  // TODO(sloretz) this could be more efficient by querying the topic ids first
  // and using those to query the messages.
  std::string sql("SELECT messages.id, messages.time_recv, topics.name,"
    " message_types.name, messages.message FROM messages JOIN topics ON"
    " topics.id = messages.topic_id JOIN message_types ON"
    " message_types.id = topics.message_type_id"
    " WHERE topics.name IN (?");

  // Build a template for the list of topics
  for (std::size_t i = 1; i < _topics.size(); i++)
  {
    sql += ", ?";
  }

  sql += ") ORDER BY messages.time_recv;";

  std::unique_ptr<raii_sqlite3::Statement> statement(
      new raii_sqlite3::Statement(*(this->dataPtr->db), sql));
  if (!*statement)
  {
    ignerr << "Failed to query messages: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
    return MsgIter();
  }

  // Bind the topic names to the statement
  int i = 1;
  int returnCode;
  for (const std::string &name : _topics)
  {
    returnCode = sqlite3_bind_text(
        statement->Handle(), i, name.c_str(), name.size(), SQLITE_TRANSIENT);
    if (returnCode != SQLITE_OK)
    {
      ignerr << "Failed to query messages: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
      return MsgIter();
    }
    ++i;
  }

  std::unique_ptr<MsgIterPrivate> msgPriv(new MsgIterPrivate);
  msgPriv->statement = std::move(statement);
  return MsgIter(std::move(msgPriv));
}

//////////////////////////////////////////////////
std::vector<Log::NameTypePair> Log::AllTopics()
{
  std::vector<Log::NameTypePair> allTopics;
  const char *sql = "SELECT topics.name, message_types.name FROM topics"
    " JOIN message_types ON topics.message_type_id = message_types.id;";
  raii_sqlite3::Statement statement(*(this->dataPtr->db), sql);
  if (!statement)
  {
    ignerr << "Failed to query topics: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
    return allTopics;
  }

  int returnCode = SQLITE_ROW;
  while (returnCode != SQLITE_DONE)
  {
    returnCode = sqlite3_step(statement.Handle());
    if (returnCode == SQLITE_ROW)
    {
      // Topic name
      const unsigned char *topic = sqlite3_column_text(statement.Handle(), 0);
      std::size_t numTopic = sqlite3_column_bytes(statement.Handle(), 0);

      // Message type name
      const unsigned char *type = sqlite3_column_text(statement.Handle(), 1);
      std::size_t numType = sqlite3_column_bytes(statement.Handle(), 1);

      // make pair
      std::string topicName(reinterpret_cast<const char*>(topic), numTopic);
      std::string topicType(reinterpret_cast<const char*>(type), numType);

      allTopics.push_back(std::make_pair(topicName, topicType));
    }
    else if (returnCode != SQLITE_DONE)
    {
      ignerr << "Failed to get topics [" << sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n";
    }
  }
  return allTopics;
}
