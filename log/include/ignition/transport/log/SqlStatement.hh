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

#ifndef IGNITION_TRANSPORT_LOG_SQLSTATEMENT_HH_
#define IGNITION_TRANSPORT_LOG_SQLSTATEMENT_HH_

#include <memory>
#include <string>
#include <vector>

#include <ignition/transport/config.hh>
#include <ignition/transport/log/Export.hh>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      // Inline bracket to help doxygen filtering.
      inline namespace IGNITION_TRANSPORT_VERSION_NAMESPACE {
      //
      /// \brief A class which contains a SQL statement parameter. SqlParameter
      /// can be useful for keeping SQL queries sanitized and avoid SQL
      /// injection. With C++17, it may be desirable to replace this class with
      /// std::variant.
      ///
      /// These parameter types map directly to the datatypes of SQLite3:
      /// https://www.sqlite.org/datatype3.html
      class IGNITION_TRANSPORT_LOG_VISIBLE SqlParameter
      {
        /// \brief An enum to indicate which type of parameter this is.
        public: enum class ParamType : int
        {
          NULL_TYPE,
          INTEGER,
          REAL,
          TEXT
          // TODO(anyone): Consider supporting blob types
        };

        /// \sa Set(std::nullptr_t)
        /// \sa SqlParameter(std::nullptr_t)
        public: SqlParameter();

        /// \sa Set(std::nullptr_t)
        /// \brief Construct NULL parameter
        public: explicit SqlParameter(std::nullptr_t);

        /// \sa Set(int64_t)
        /// \brief Construct integer parameter
        /// \param[in] _integer an integer
        public: explicit SqlParameter(int64_t _integer);

        /// \sa Set(double)
        /// \brief Construct real parameter
        /// \param[in] _real a real number
        public: explicit SqlParameter(double _real);

        /// \sa Set(const std::string &)
        /// \brief Construct string parameter
        /// \param[in] _string a string
        public: explicit SqlParameter(const std::string &_string);

        /// \brief Copy constructor
        /// \param[in] _other Another SqlParameter
        public: SqlParameter(const SqlParameter &_other);

        /// \brief Move constructor
        /// \param[in] _other Another SqlParameter
        public: SqlParameter(SqlParameter &&_other);  // NOLINT

        /// \brief Copy assignment operator
        /// \param[in] _other Another SqlParameter
        /// \return This object
        public: SqlParameter &operator=(const SqlParameter &_other);

        /// \brief Move assignment operator
        /// \param[in] _other Another SqlParameter
        /// \return This object
        public: SqlParameter &operator=(SqlParameter &&_other);  // NOLINT

        /// \brief Set this parameter to a NULL_TYPE.
        /// \param[in] _np Pass in a nullptr.
        public: void Set(std::nullptr_t _np);

        /// \brief Set this parameter to an INTEGER type.
        /// \param[in] _integer The integer value to set this parameter to.
        public: void Set(int64_t _integer);

        /// \brief Set this parameter to a floating-point type.
        /// \param[in] _real The value to set this parameter to.
        public: void Set(double _real);

        /// \brief Set this parameter to a text string.
        /// \param[in] _text The value to set this parameter to.
        public: void Set(const std::string &_text);

        /// \brief Get the type for this parameter.
        /// \return The type for this parameter
        public: ParamType Type() const;

        /// \brief Get the integer value of this parameter.
        /// \return A pointer to this parameter's integer value, or a nullptr
        /// if it does not contain an integer.
        public: const int64_t *QueryInteger() const;

        /// \brief Get the floating-point value of this parameter.
        /// \return A pointer to this parameter's floating-point value, or a
        /// nullptr if it does not contain a REAL value.
        public: const double *QueryReal() const;

        /// \brief Get the text value of this paramter.
        /// \return A pointer to this parameter's text string, or a nullptr if
        /// it does not contain a text string.
        public: const std::string *QueryText() const;

        /// \brief Destructor
        public: ~SqlParameter();

        /// \internal Implementation of this class
        private: class Implementation;

#ifdef _WIN32
// Disable warning C4251 which is triggered by
// std::*
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
        /// \internal Pointer to implementation
        private: std::unique_ptr<Implementation> dataPtr;
#ifdef _WIN32
#pragma warning(pop)
#endif
      };

      /// \brief A statement for a SQL query. These are generated by the
      /// QueryOptions class to control how messages get queried from the log.
      struct IGNITION_TRANSPORT_LOG_VISIBLE SqlStatement
      {
#ifdef _WIN32
// Disable warning C4251 which is triggered by
// std::*
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
        /// \brief Text of the statement
        public: std::string statement;

        /// \brief Parameters for the statement
        public: std::vector<SqlParameter> parameters;
#ifdef _WIN32
#pragma warning(pop)
#endif

        /// \brief Append another SQL statement onto the end of this one.
        /// _other.statement will be added to the end of this object's statement
        /// string, and _other's parameters will be added to this object's
        /// vector of parameters.
        /// \param[in] _other Another SQL statement.
        public: void Append(const SqlStatement &_other);
      };
      }
    }
  }
}

#endif
