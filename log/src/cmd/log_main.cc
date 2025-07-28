/*
 * Copyright (C) 2025 Open Source Robotics Foundation
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

#include <gz/utils/cli/Error.hpp>
#include <iostream>
#include <memory>
#include <string>

#include <gz/utils/cli/CLI.hpp>
#include <gz/utils/cli/GzFormatter.hpp>

#include <gz/transport/config.hh>
#include "LogCommandAPI.hh"

//////////////////////////////////////////////////
/// \brief Structure to hold all available log options
struct LogOptions
{
  /// \brief Log filename
  std::string file{""};

  /// \brief Regex pattern
  std::string pattern{".+"};

  /// \brief Flag to for file overwrite
  int force{0};

  /// \brief Remap of topics
  std::string remap{""};

  /// \brief Wait time between topic advertisement (in ms)
  int wait{1000};

  /// \brief Flag to enable fast playback
  int fast{0};
};

//////////////////////////////////////////////////
void addCommonFlags(CLI::App *_app)
{
  _app->add_option_function<int>("-v,--verbose",
    [](const int _verbosity){
      verbosity(_verbosity);
    },
    "Set the verbosity level")
    ->expected(0, 1)
    ->default_val(1);
}

//////////////////////////////////////////////////
void addRecordFlags(CLI::App *_app)
{
  auto opt = std::make_shared<LogOptions>();

  auto command = _app->add_option_group("command", "Command to be executed.");

  command->add_option("--file", opt->file,
                      "Name of the log file. Default name\n"
                      "will be <datetime>.tlog");

  command->add_flag("--force", opt->force,
                    "Overwrite a file if it exists.");

  command->add_option("--pattern", opt->pattern,
                      "Regular expression in C++ ECMAScript grammar.\n"
                      "Default is match all topics.");

  _app->callback(
    [opt](){
      recordTopics(opt->file.c_str(), opt->pattern.c_str(), opt->force);
    });
}

//////////////////////////////////////////////////
void addPlaybackFlags(CLI::App *_app)
{
  auto opt = std::make_shared<LogOptions>();

  auto command = _app->add_option_group("command", "Command to be executed.");

  command->add_option("--file", opt->file,
                      "Name of the log file.")
                      ->required();

  command->add_option("--pattern", opt->pattern,
                      "Regular expression in C++ ECMAScript grammar.\n"
                      "Default is match all topics.");

  command->add_option("--remap", opt->remap,
                      "Rename a topic while playing back. Use := to\n"
                      "separate the two topics as shown, FROM:=TO");

  command->add_option("--wait", opt->wait,
                      "Time to wait (in milliseconds) between topic\n"
                      "advertisement and publishing.")
                      ->default_val(1000);

  command->add_flag("-f", opt->fast,
                    "Enable fast playback that will publish messages\n"
                    "without waiting between messages according to\n"
                    "the logged timestamps.");

  _app->callback(
      [opt](){
        playbackTopics(opt->file.c_str(), opt->pattern.c_str(),
                       opt->wait, opt->remap.c_str(), opt->fast);
      });
}

//////////////////////////////////////////////////
int main(int argc, char** argv)
{
  CLI::App app{"Record and playback Gazebo Transport topics"};

  app.add_flag_callback("-v,--version", [](){
      std::cout << GZ_TRANSPORT_VERSION_FULL << std::endl;
      throw CLI::Success();
  });

  auto recordApp = app.add_subcommand("record",
                                      "Record Gazebo Transport topics");
  addCommonFlags(recordApp);
  addRecordFlags(recordApp);

  auto playbackApp = app.add_subcommand("playback",
                                        "Playback previously recorded Gazebo "
                                        "Transport topics");
  addCommonFlags(playbackApp);
  addPlaybackFlags(playbackApp);

  app.formatter(std::make_shared<GzFormatter>(&app));
  CLI11_PARSE(app, argc, argv);
}

