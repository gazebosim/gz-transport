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

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <ignition/msgs/Factory.hh>

#include "ignition/transport/log/Descriptor.hh"
#include "ignition/transport/log/Log.hh"
#include "ignition/transport/log/SqlStatement.hh"
#include "BatchPrivate.hh"
#include "build_config.hh"
#include "Console.hh"
#include "Descriptor.hh"
#include "raii-sqlite3.hh"

using namespace ignition::transport;
using namespace ignition::transport::log;

/// \brief Private implementation
class ignition::transport::log::Log::Implementation
{
  /// \internal \sa Log::Descriptor()
  public: const log::Descriptor *Descriptor() const;

  /// \brief End transaction if enough time has passed since it began
  /// \return one of the SQLite error codes
  public: int EndTransactionIfEnoughTimeHasPassed();

  /// \brief End transaction immediately
  /// \return one of the SQLite error codes
  public: int EndTransaction();

  /// \brief Begin transaction if one isn't already open
  /// \return one of the SQLite error codes
  public: int BeginTransactionIfNotInOne();

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

  public: void InsertDescriptor(
              const google::protobuf::Descriptor *_desc);

  /// \brief Get whether a message type exists in the database.
  /// \param[in] _msgTypeName Message typename to check.
  /// \return True if the message type name exists in the database.
  public: bool HasMessageType(const std::string &_msgTypeName);

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

  /// \brief Name of the log file.
  public: std::string filename = "";

  /// \brief Time of the first message in the log file.
  public: std::chrono::nanoseconds startTime = std::chrono::nanoseconds(-1);

  /// \brief Time of the last message in the log file.
  public: std::chrono::nanoseconds endTime = std::chrono::nanoseconds(-1);
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
    const char * sql =
      "SELECT topics.id, topics.name, message_types.name FROM topics"
      " JOIN message_types ON topics.message_type_id = message_types.id;";

    raii_sqlite3::Statement topicStatement(*(this->db), sql);
    if (!topicStatement)
    {
      LERR("Failed to compile statement to get topic ids\n");
      return nullptr;
    }

    // Execute the statement
    int returnCode;
    do
    {
      returnCode = sqlite3_step(topicStatement.Handle());
      if (returnCode == SQLITE_ROW)
      {
        // get the data about the topic
        sqlite_int64 topicId = sqlite3_column_int64(
            topicStatement.Handle(), 0);

        const unsigned char *topicName = sqlite3_column_text(
            topicStatement.Handle(), 1);
        std::size_t lenTopicName = sqlite3_column_bytes(
            topicStatement.Handle(), 1);

        const unsigned char *typeName = sqlite3_column_text(
            topicStatement.Handle(), 2);
        std::size_t lenTypeName = sqlite3_column_bytes(
            topicStatement.Handle(), 2);

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
int Log::Implementation::EndTransactionIfEnoughTimeHasPassed()
{
  if (!this->TimeForNewTransaction())
  {
    return SQLITE_OK;
  }

  // End the transaction
  return this->EndTransaction();
}

//////////////////////////////////////////////////
int Log::Implementation::EndTransaction()
{
  // End the transaction
  int returnCode = sqlite3_exec(
      this->db->Handle(), "END;", NULL, 0, nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to end transaction" << returnCode << "\n");
    return returnCode;
  }
  LDBG("Ended transaction\n");
  this->inTransaction = false;
  return returnCode;
}

//////////////////////////////////////////////////
int Log::Implementation::BeginTransactionIfNotInOne()
{
  if (this->inTransaction)
    return SQLITE_OK;

  int returnCode = sqlite3_exec(
      this->db->Handle(), "BEGIN;", NULL, 0, nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to begin transaction" << returnCode << "\n");
    return returnCode;
  }
  this->inTransaction = true;
  LDBG("Began transaction\n");
  this->lastTransaction = std::chrono::steady_clock::now();
  return returnCode;
}

//////////////////////////////////////////////////
bool Log::Implementation::TimeForNewTransaction() const
{
  auto now = std::chrono::steady_clock::now();
  return now - this->transactionPeriod > this->lastTransaction;
}

//////////////////////////////////////////////////
bool Log::Implementation::HasMessageType(const std::string &_msgTypeName)
{
  // Compile the statement
  const char* const  msgTypeStatement =
      "SELECT id FROM message_types WHERE name = ?001 LIMIT 1;";
  raii_sqlite3::Statement statement(*(this->db), msgTypeStatement);
  if (!statement)
  {
    LERR("Failed to compile message type query statement\n");
    return false;
  }

  // Bind message type name parameter
  int returnCode = sqlite3_bind_text(
      statement.Handle(), 1, _msgTypeName.c_str(),
      _msgTypeName.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind message type name(1): " << returnCode << "\n");
    return -1;
  }

  // Try to run it
  int resultCode = sqlite3_step(statement.Handle());
  if (resultCode == SQLITE_CORRUPT)
  {
    LERR("Database is corrupt.");
  }
  else if (resultCode != SQLITE_ROW)
  {
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
void Log::Implementation::InsertDescriptor(
    const google::protobuf::Descriptor *_descriptor)
{
  if (!_descriptor)
    return;

  // Lambda function that inserts the message type and protobuf
  // definition into the database.
  std::function<void(const google::protobuf::Descriptor *)> insertDescriptor =
  [&](const google::protobuf::Descriptor *_desc)->void
  {
    const std::string msgTypeName = _desc->full_name();
    if (!this->HasMessageType(msgTypeName))
    {
      std::cout << "Inserting type[" << msgTypeName << "]\n";
      const std::string descriptorStr = _desc->file()->DebugString();

      const std::string sqlMessageType =
        "INSERT INTO message_types (name, proto_descriptor) "
        "VALUES (?001, ?002);";

      raii_sqlite3::Statement messageTypeStatement(
          *(this->db), sqlMessageType);

      if (!messageTypeStatement)
      {
        LERR("Failed to compile statement to insert message type\n");
        return;
      }

      int returnCode;

      // Bind message type name parameter
      returnCode = sqlite3_bind_text(
          messageTypeStatement.Handle(), 1, msgTypeName.c_str(),
          msgTypeName.size(), nullptr);
      if (returnCode != SQLITE_OK)
      {
        LERR("Failed to bind message type name(1): " << returnCode << "\n");
        return;
      }

      // Bind protobuf descriptor parameter
      returnCode = sqlite3_bind_text(
          messageTypeStatement.Handle(), 2, descriptorStr.c_str(),
          descriptorStr.size(), nullptr);
      if (returnCode != SQLITE_OK)
      {
        LERR("Failed to bind message proto_descriptor(2): "
            << returnCode << "\n");
        return;
      }

      returnCode = sqlite3_step(messageTypeStatement.Handle());
      if (returnCode != SQLITE_DONE)
      {
        LERR("Failed to insert message type. sqlite3 return code[" << returnCode
            << "] message type[" << msgTypeName << "]\n");
        return;
      }
    }
    else
    {
      std::cout << "Already inserted type[" << msgTypeName << "]\n";
    }
  };

  // Recursive lambda function that will add all field message types.
  // This makes sure that a message is fully defined in the database.
  std::function<void(const google::protobuf::Descriptor *,
      std::vector<std::string> &_types)> descriptors =
    [&](const google::protobuf::Descriptor *_desc,
                   std::vector<std::string> &_types)->void
  {
    // We keep track of message types to improve insertion performance
    if (_desc && std::find(_types.begin(), _types.end(), _desc->full_name()) ==
        _types.end())
    {
      _types.push_back(_desc->full_name());

      insertDescriptor(_desc);

      for (int i = 0; i < _desc->field_count(); ++i)
      {
        const google::protobuf::FieldDescriptor *fd = _desc->field(i);
        descriptors(fd->message_type(), _types);
      }
    }
  };

  std::vector<std::string> types;
  descriptors(_descriptor, types);
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

  std::cout << "\n Insert or get topic id[" << _name << "  " << _type << "]\n";
  std::unique_ptr<google::protobuf::Message> msg = msgs::Factory::New(_type);
  const google::protobuf::Descriptor *pDesc = msg->GetDescriptor();
  this->InsertDescriptor(pDesc);

  // Inserting a new topic invalidates the descriptor
  this->needNewDescriptor = true;

  // Otherwise insert it into the database and return the new topic_id
  const std::string sqlTopic =
    "INSERT INTO topics (name, message_type_id)"
    " SELECT ?002, id FROM message_types WHERE name = ?001 LIMIT 1;";

  raii_sqlite3::Statement topicStatement(
      *(this->db), sqlTopic);
  if (!topicStatement)
  {
    LERR("Failed to compile statement to insert topic\n");
    return -1;
  }

  // Reset startTime and endTime
  this->startTime = std::chrono::nanoseconds(-1);
  this->endTime = std::chrono::nanoseconds(-1);

  int returnCode;
  // Bind parameters
  returnCode = sqlite3_bind_text(
      topicStatement.Handle(), 1, _type.c_str(), _type.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind message type name(2): " << returnCode << "\n");
    return -1;
  }
  returnCode = sqlite3_bind_text(
      topicStatement.Handle(), 2, _name.c_str(), _name.size(), nullptr);
  if (returnCode != SQLITE_OK)
  {
    LERR("Failed to bind topic name: " << returnCode << "\n");
    return -1;
  }

  // Execute the statement
  returnCode = sqlite3_step(topicStatement.Handle());
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
    const int64_t _topic,
    const void *_data,
    const std::size_t _len)
{
  // \todo Record and playback empty messages. A topic could publish an
  // Int32 or Int64 message with data=0. In this situation the protobuf
  // message has size zero. While this is not a problem for protobuf messages,
  // it is considered an error for SQLite3. We short circuit here in order to
  // prevent the final LERR in this function from spamming the console.
  if (_len == 0)
    return false;

  int returnCode;
  const std::string sql =
    "INSERT INTO messages (time_recv, message, topic_id)"
    "VALUES (?001, ?002, ?003);";

  // Compile the statement
  raii_sqlite3::Statement statement(*(this->db), sql);
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

  // Reset startTime and endTime
  this->startTime = std::chrono::nanoseconds(-1);
  this->endTime = std::chrono::nanoseconds(-1);

  // Execute the statement
  returnCode = sqlite3_step(statement.Handle());
  if (returnCode != SQLITE_DONE)
  {
    LERR("Failed to insert message. sqlite3 return code[" << returnCode
        << "] data[" << _data << "] len[" << _len << "]\n");
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
    this->dataPtr->EndTransaction();
  }
}

//////////////////////////////////////////////////
bool Log::Valid() const
{
  return this->dataPtr && this->dataPtr->db && *(this->dataPtr->db);
}

//////////////////////////////////////////////////
bool Log::Open(const std::string &_file, const std::ios_base::openmode _mode)
{
  // Open the SQLite3 database
  if (this->dataPtr->db)
  {
    LERR("A database is already open\n");
    return false;
  }
  int64_t modeSQL = SQLITE_OPEN_URI;
  if (std::ios_base::out & _mode)
  {
    modeSQL = modeSQL | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  }
  else if (std::ios_base::in & _mode)
  {
    modeSQL = modeSQL | SQLITE_OPEN_READONLY;
  }

  std::unique_ptr<raii_sqlite3::Database> db(
      new raii_sqlite3::Database(_file, modeSQL));
  if (!*(db))
  {
    // The constructor of raii_sqlite3::Database will print out the reason that
    // the database failed to open.
    return false;
  }

  // Don't need to create a schema if this is read only
  if (std::ios_base::out & _mode)
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

  this->dataPtr->filename = _file;
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
    const void *_data, const std::size_t _len)
{
  if (!this->Valid())
  {
    return false;
  }

  // Need to insert multiple messages pertransaction for best performance
  if (SQLITE_OK != this->dataPtr->BeginTransactionIfNotInOne())
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
  if (SQLITE_OK != this->dataPtr->EndTransactionIfEnoughTimeHasPassed())
  {
    // Something is really busted if this happens
    LERR("Failed to end transcation: "<< sqlite3_errmsg(
        this->dataPtr->db->Handle()) << "\n");
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
Batch Log::QueryMessages(const QueryOptions &_options)
{
  const log::Descriptor *desc = this->Descriptor();

  // Make sure the log has been initialized.
  // TODO(anyone): Should we print a warning here?
  if (!desc)
    return Batch();

  std::unique_ptr<BatchPrivate> batchPriv(
        new BatchPrivate(this->dataPtr->db,
                         _options.GenerateStatements(*desc)));

  return Batch(std::move(batchPriv));
}

//////////////////////////////////////////////////
std::chrono::nanoseconds Log::StartTime() const
{
  // Short circuit if we already looked up the start time once.
  if (this->dataPtr->startTime >= std::chrono::nanoseconds::zero())
    return this->dataPtr->startTime;

  this->dataPtr->startTime = std::chrono::nanoseconds::zero();

  if (!this->Valid())
  {
    LERR("Cannot get start time of an invalid log.\n");
    return this->dataPtr->startTime;
  }

  // Compile the statement
  const char* const getStartTimeStatement =
      "SELECT MIN(time_recv) AS start_time FROM messages;";
  raii_sqlite3::Statement statement(*(this->dataPtr->db),
                                    getStartTimeStatement);
  if (!statement)
  {
    LERR("Failed to compile start time query statement\n");
    return this->dataPtr->startTime;
  }

  // Try to run it
  int resultCode = sqlite3_step(statement.Handle());
  if (resultCode == SQLITE_CORRUPT)
  {
    LERR("Database is corrupt, playback may fail or be truncated.");
  }
  else if (resultCode != SQLITE_ROW)
  {
    LERR("Database has no messages\n");
    return this->dataPtr->startTime;
  }

  // Return start time found.
  sqlite_int64 startTimeAsInt = sqlite3_column_int64(statement.Handle(), 0);
  this->dataPtr->startTime = std::chrono::nanoseconds(startTimeAsInt);
  return this->dataPtr->startTime;
}

//////////////////////////////////////////////////
std::chrono::nanoseconds Log::EndTime() const
{
  // Short circuit if we already looked up the end time once.
  if (this->dataPtr->endTime >= std::chrono::nanoseconds::zero())
    return this->dataPtr->endTime;

  this->dataPtr->endTime = std::chrono::nanoseconds::zero();

  if (!this->Valid())
  {
    LERR("Cannot get end time of an invalid log.\n");
    return this->dataPtr->endTime;
  }

  // Compile the statement
  const char* const getEndTimeStatement =
      "SELECT MAX(time_recv) AS end_time FROM messages;";
  raii_sqlite3::Statement statement(*(this->dataPtr->db),
                                    getEndTimeStatement);
  if (!statement)
  {
    LERR("Failed to compile end time query statement\n");
    return this->dataPtr->endTime;
  }

  // Try to run it
  int resultCode = sqlite3_step(statement.Handle());
  sqlite_int64 endTimeAsInt = 0;

  if (resultCode == SQLITE_CORRUPT)
  {
    LERR("Database is corrupt, retrieving last valid message." \
         "Playback may fail or be truncated.");

    // If the database is corrupt, then we need to iterate over the valid
    // messages until we get the to corrupt statement. The timestamp
    // of the last valid message is returned.
    const char* const getAllTimesStatement =
      "SELECT time_recv AS end_time FROM messages;";
    raii_sqlite3::Statement statementAll(*(this->dataPtr->db),
        getAllTimesStatement);

    if (!statementAll)
    {
      LERR("Failed to compile end time all query statement\n");
      return this->dataPtr->endTime;
    }

    // Iterate until we get to the corrupt line.
    while (sqlite3_step(statementAll.Handle()) != SQLITE_CORRUPT)
      endTimeAsInt = sqlite3_column_int64(statementAll.Handle(), 0);
  }
  else if (resultCode != SQLITE_ROW)
  {
    LERR("Database has no messages\n");
  }
  else
  {
    endTimeAsInt = sqlite3_column_int64(statement.Handle(), 0);
  }

  this->dataPtr->endTime = std::chrono::nanoseconds(endTimeAsInt);

  // Return end time found.
  return this->dataPtr->endTime;
}

//////////////////////////////////////////////////
std::string Log::Version() const
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

//////////////////////////////////////////////////
std::string Log::Filename() const
{
  return this->dataPtr->filename;
}
