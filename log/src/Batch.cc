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

#include <ignition/common/Console.hh>

#include "ignition/transport/log/Batch.hh"
#include "src/BatchPrivate.hh"
#include "src/MsgIterPrivate.hh"
#include "src/raii-sqlite3.hh"
#include "build_config.hh"


using namespace ignition::transport;
using namespace ignition::transport::log;


//////////////////////////////////////////////////
Batch::Batch()
  : dataPtr(new BatchPrivate)
{
}

//////////////////////////////////////////////////
Batch::Batch(Batch &&_old)  // NOLINT
  : dataPtr(std::move(_old.dataPtr))
{
}

//////////////////////////////////////////////////
Batch::~Batch()
{
}

//////////////////////////////////////////////////
MsgIter Batch::begin()
{
  std::unique_ptr<MsgIterPrivate> msgPriv(new MsgIterPrivate);
  auto statementPtr = this->dataPtr->CreateStatement();
  if (!statementPtr)
    return MsgIter();
  msgPriv->statement = std::move(statementPtr);
  return MsgIter(std::move(msgPriv));
}

//////////////////////////////////////////////////
MsgIter Batch::end()
{
  return MsgIter();
}

//////////////////////////////////////////////////
Batch::Batch(std::unique_ptr<BatchPrivate> &&_pimpl)
  : dataPtr(std::move(_pimpl))
{
}

//////////////////////////////////////////////////
std::unique_ptr<raii_sqlite3::Statement> BatchPrivate::CreateStatement()
{
  if (!this->db)
    return nullptr;

  if (!this->topicNames.empty())
  {
    // Filter messages by topic name
    // TODO(sloretz) this could be more efficient by querying by the topic ids
    std::string sql("SELECT messages.id, messages.time_recv, topics.name,"
      " message_types.name, messages.message FROM messages JOIN topics ON"
      " topics.id = messages.topic_id JOIN message_types ON"
      " message_types.id = topics.message_type_id"
      " WHERE topics.name IN (?");

    // Build a template for the list of topics
    for (std::size_t i = 1; i < this->topicNames.size(); i++)
    {
      sql += ", ?";
    }

    sql += ") ORDER BY messages.time_recv;";

    std::unique_ptr<raii_sqlite3::Statement> statement(
        new raii_sqlite3::Statement(*(this->db), sql));
    if (!*statement)
    {
      ignerr << "Failed to query messages: "<< sqlite3_errmsg(
          this->db->Handle()) << "\n";
      return nullptr;
    }

    // Bind the topic names to the statement
    int i = 1;
    int returnCode;
    for (const std::string &name : this->topicNames)
    {
      returnCode = sqlite3_bind_text(
          statement->Handle(), i, name.c_str(), name.size(), SQLITE_TRANSIENT);
      if (returnCode != SQLITE_OK)
      {
        ignerr << "Failed to query messages: "<< sqlite3_errmsg(
          this->db->Handle()) << "\n";
        return nullptr;
      }
      ++i;
    }

    return std::move(statement);
  }
  else
  {
    // Default to all messages
    const char *sql = "SELECT messages.id, messages.time_recv, topics.name,"
      " message_types.name, messages.message FROM messages JOIN topics ON"
      " topics.id = messages.topic_id JOIN message_types ON"
      " message_types.id = topics.message_type_id ORDER BY messages.time_recv;";
    std::unique_ptr<raii_sqlite3::Statement> statement(
        new raii_sqlite3::Statement(*(this->db), sql));
    if (!*statement)
    {
      ignerr << "Failed to query messages: "<< sqlite3_errmsg(
          this->db->Handle()) << "\n";
      return nullptr;
    }
    return std::move(statement);
  }

  return nullptr;
}
