cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

project(gz-transport-benchmark)

find_package(gz-transport QUIET REQUIRED)

########################################
# bench — requires GFlags
find_library(gflags_LIBRARIES NAMES gflags)
find_path(gflags_INCLUDE_DIRS gflags/gflags.h ENV CPATH)
if (NOT gflags_LIBRARIES OR NOT gflags_INCLUDE_DIRS)
  message(STATUS "Unable to compile 'bench', missing GFlags library")
else()
  include_directories(SYSTEM ${gflags_INCLUDE_DIRS})
  add_executable(bench bench.cc)
  target_link_libraries(bench
    gz-transport::core
    ${gflags_LIBRARIES})
endif()
