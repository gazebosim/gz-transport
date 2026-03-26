cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)

project(ignition-transport-benchmark)

find_package(ignition-transport11 QUIET REQUIRED)
set(IGN_TRANSPORT_VER ${ignition-transport11_VERSION_MAJOR})

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
    ignition-transport${IGN_TRANSPORT_VER}::core
    ${gflags_LIBRARIES})
endif()
