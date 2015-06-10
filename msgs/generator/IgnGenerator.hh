/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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
#ifndef _IGN_TRANSPORT_GENERATOR_HH_
#define _IGN_TRANSPORT_GENERATOR_HH_

#include <google/protobuf/compiler/code_generator.h>
#include <string>

namespace google
{
  namespace protobuf
  {
    namespace compiler
    {
      namespace cpp
      {
        class GeneratorContext;

        /// \brief Google protobuf message generator for Ign::msgs
        class IgnGenerator : public CodeGenerator
        {
          /// \brief Class constructor.
          public: IgnGenerator() = default;

          /// \brief Class destructor.
          public: virtual ~IgnGenerator() = default;

          /// \brief Generates code for the given proto file, generating one or
          /// more files in the given output directory. This method also ignores
          /// some warnings and creates extra typedef definitions.
          /// \param[in] _file File descriptor (.proto)
          /// \param[in] _parameter Extra optional parameter for the generator.
          /// \param[out] _generatorContext Represents the directory to which
          /// the CodeGenerator is to write and other information about the
          /// context in which the Generator runs.
          /// \param[out] _error Description of the problem if the function
          /// failed.
          /// \return True if successful, false otherwise
          public: virtual bool Generate(const FileDescriptor *_file,
                                        const string &_parameter,
                                        OutputDirectory *_generatorContext,
                                        std::string *_error) const;
        };
        // namespace cpp
      }
      // namespace compiler
    }
    // namespace protobuf
  }
  // namespace google
}
#endif
