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
#ifndef IGNITION_TRANSPORT_LOG_BATCHPRIVATE_HH_
#define IGNITION_TRANSPORT_LOG_BATCHPRIVATE_HH_

#include <memory>
#include <string>

#include "src/raii-sqlite3.hh"

using namespace ignition::transport;
using namespace ignition::transport::log;


class ignition::transport::log::BatchPrivate
{
  /// \brief Create a sqlite3 statement
  public: std::unique_ptr<raii_sqlite3::Statement> CreateStatement();

  /// \brief topic names that should be queried
  public: std::unordered_set<std::string> topicNames;

  /// \brief SQLite3 database pointer wrapper
  public: std::shared_ptr<raii_sqlite3::Database> db;
};

#endif

