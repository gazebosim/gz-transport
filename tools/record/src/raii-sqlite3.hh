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
#ifndef IGNITION_TRANSPORT_TOOLS_RECORD_RAIISQLITE3_HH_
#define IGNITION_TRANSPORT_TOOLS_RECORD_RAIISQLITE3_HH_

#include <iostream>

#include <sqlite3.h>

/// \internal
/// \remarks Not using PIMPL because these classes are for internal use only
namespace raii_sqlite3
{
  /// \brief Thin RAII wrapper for a (sqlite3 *)
  /// Automatically calls sqlite_close()
  /// \internal
  class Database
  {
    /// \brief move constructor
    public: Database(Database &&_other)
    {
      this->handle = _other.handle;
      _other.handle = nullptr;
    }

    /// \brief Constructor
    /// \param[in] _path UTF-8 path to db file to open
    /// \param[in] _flags Flags to use when opening the database
    public: Database(std::string _path, int _flags)
    {
      // Open the database;
      int return_code = sqlite3_open_v2(
          _path.c_str(), &(this->handle), _flags, nullptr);

      if (return_code != SQLITE_OK)
      {
        sqlite3_close(this->handle);
        this->handle = nullptr;
        return;
      }

      // Turn on extended error codes
      return_code = sqlite3_extended_result_codes(this->handle, 1);
      if (return_code != SQLITE_OK)
      {
        sqlite3_close(this->handle);
        this->handle = nullptr;
        return;
      }

      // Turn on foreign key support
      const char *sql = "PRAGMA foreign_keys = ON;";
      return_code = sqlite3_exec(this->handle, sql, NULL, 0, NULL);
      if (return_code != SQLITE_OK)
      {
        sqlite3_close(this->handle);
        this->handle = nullptr;
        return;
      }
    }

    /// \brief Destructor
    public: ~Database()
    {
      if (this->handle)
      {
        sqlite3_close(this->handle);
      }
    }

    /// \brief Handle
    public: sqlite3 *Handle()
    {
      return this->handle;
    }

    /// \brief Return true if the database is valid.
    operator bool() const
    {
      return this->handle != nullptr;
    }

    /// \brief the pointer this is wrapping
    protected: sqlite3 *handle = nullptr;
  };

  /// \brief Thin RAII wrapper for a (sqlite3_stmt *)
  /// Automatically calls sqlite_finalize()
  /// \internal
  class Statement
  {
    /// \brief Constructor
    /// \param[in] _db The database the statement is being used on
    /// \param[in] _sql A single SQL statement to compile
    public: Statement(Database &_db, const std::string &_sql)
    {
      int return_code = sqlite3_prepare_v2(
          _db.Handle(), _sql.c_str(), _sql.size(), &(this->handle), NULL);

      if (return_code != SQLITE_OK)
      {
        if (this->handle)
        {
          sqlite3_finalize(this->handle);
          this->handle = nullptr;
        }
        return;
      }
    }

    /// \brief Address-of operator
    /// \brief Handle
    public: sqlite3_stmt *Handle()
    {
      return this->handle;
    }

    /// \brief Destructor
    public: ~Statement()
    {
      if (this->handle)
      {
        sqlite3_finalize(this->handle);
      }
    }

    /// \brief Return true if the statement is valid is valid.
    operator bool() const
    {
      return this->handle != nullptr;
    }

    /// \brief the pointer this is wrapping
    protected: sqlite3_stmt *handle = nullptr;
  };
}

#endif
