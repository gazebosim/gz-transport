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

#include <ignition/common/Console.hh>

#include "ignition/transport/log/MsgIter.hh"
#include "src/MsgIterPrivate.hh"
#include "src/raii-sqlite3.hh"

using namespace ignition::transport;
using namespace ignition::transport::log;


//////////////////////////////////////////////////
void MsgIterPrivate::StepStatement()
{
  if (this->statement)
  {
    // Get the results from the statement
    int returnCode = sqlite3_step(this->statement->Handle());

    if (returnCode == SQLITE_ROW)
    {
      // TODO get data and create message in the dereference operators
      // Assumes statement has column order:
      // messages id (0), timeRecv(1), topics name(2), 
      // message_type name(3), message data(4)
      ignition::common::Time timeRecv;

      // Time received
      sqlite_int64 timeRecvInt = sqlite3_column_int64(
          this->statement->Handle(), 1);
      timeRecv.sec = timeRecvInt / 1000000000;
      timeRecv.nsec = timeRecvInt % 1000000000;

      // Topic name
      const unsigned char *topic = sqlite3_column_text(
          this->statement->Handle(), 2);
      std::size_t numTopic = sqlite3_column_bytes(
          this->statement->Handle(), 2);

      // Message type name
      const unsigned char *type = sqlite3_column_text(
          this->statement->Handle(), 3);
      std::size_t numType = sqlite3_column_bytes(this->statement->Handle(), 3);

      // Message data
      const void *data = sqlite3_column_blob(this->statement->Handle(), 4);
      std::size_t numData = sqlite3_column_bytes(this->statement->Handle(), 4);

      this->message.reset(new Message(
            timeRecv,
            data, numData,
            reinterpret_cast<const char*>(type), numType,
            reinterpret_cast<const char*>(topic), numTopic));
    }
    else
    {
      if (returnCode != SQLITE_DONE)
      {
        ignerr << "Failed to get message [" << returnCode << "]\n";
      }
      // Out of data
      this->statement.reset();
    }
  }
}

//////////////////////////////////////////////////
MsgIter::MsgIter()
  : dataPtr(new MsgIterPrivate)
{
}

//////////////////////////////////////////////////
// TODO
// MsgIter::MsgIter(const MsgIter &_orig)
//   : dataPtr(new MsgIterPrivate)
// {
//   // TODO this copy constructor needs to create a new statement starting
//   // at the current row id
// }

MsgIter::MsgIter(MsgIter &&_orig)
  : dataPtr(std::move(_orig.dataPtr))
{
}

//////////////////////////////////////////////////
MsgIter::MsgIter(std::unique_ptr<MsgIterPrivate> &&_pimpl)
  : dataPtr(std::move(_pimpl))
{
  this->dataPtr->StepStatement();
}

//////////////////////////////////////////////////
MsgIter::~MsgIter()
{
}

//////////////////////////////////////////////////
// TODO
// MsgIter &MsgIter::operator=(const MsgIter &_orig)
// {
//   dataPtr.reset(new MsgIterPrivate(*(_orig.dataPtr)));
// }

//////////////////////////////////////////////////
MsgIter &MsgIter::operator++()
{
  this->dataPtr->StepStatement();
  return *this;
}

//////////////////////////////////////////////////
// TODO
// MsgIter MsgIter::operator++(int)
// {
// }

//////////////////////////////////////////////////
// TODO
bool MsgIter::operator==(const MsgIter &_other) const
{
  // TODO this won't work once this class has a propper copy constructor
  // It's only good enough to compare this with an empty iterator
  return this->dataPtr->statement.get() == _other.dataPtr->statement.get();
}

//////////////////////////////////////////////////
bool MsgIter::operator!=(const MsgIter &_other) const
{
  return !this->operator==(_other);
}

//////////////////////////////////////////////////
const Message &MsgIter::operator*() const
{
  return *this->dataPtr->message;
}

//////////////////////////////////////////////////
const Message *MsgIter::operator->() const
{
  return this->dataPtr->message.get();
}
