cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

project(ignition-transport-log-examples)

find_package(ignition-transport5 REQUIRED COMPONENTS log)

if (EXISTS "${CMAKE_SOURCE_DIR}/record.cc")
  add_executable(example_record record.cc)
  target_link_libraries(example_record ignition-transport5::log)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/playback.cc")
  add_executable(example_playback playback.cc)
  target_link_libraries(example_playback ignition-transport5::log)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/custom_query.cc")
  add_executable(example_custom_query custom_query.cc)
  target_link_libraries(example_custom_query ignition-transport5::log)
endif()
