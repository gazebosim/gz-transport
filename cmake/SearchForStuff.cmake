include (${project_cmake_dir}/Utils.cmake)
include (CheckCXXSourceCompiles)

include (${project_cmake_dir}/FindOS.cmake)
include (FindPkgConfig)

########################################
if (PROTOBUF_VERSION LESS 2.3.0)
  BUILD_ERROR("Incorrect version: Gazebo requires protobuf version 2.3.0 or greater")
endif()

########################################
# The Google Protobuf library for message generation + serialization
find_package(Protobuf REQUIRED)
if (NOT PROTOBUF_FOUND)
  BUILD_ERROR ("Missing: Google Protobuf (libprotobuf-dev)")
endif()
if (NOT PROTOBUF_PROTOC_EXECUTABLE)
  BUILD_ERROR ("Missing: Google Protobuf Compiler (protobuf-compiler)")
endif()
if (NOT PROTOBUF_PROTOC_LIBRARY)
  BUILD_ERROR ("Missing: Google Protobuf Compiler Library (libprotoc-dev)")
endif()

#################################################
# Find ZeroMQ.
find_path (zmq_INCLUDE_DIRS zmq.hpp)
set (ZMQ_FOUND True)

if (NOT zmq_INCLUDE_DIRS)
  message (STATUS "Looking for zmq.hpp - not found")
  set (ZMQ_FOUND False)
else ()
  message (STATUS "Looking for zmq.hpp - found")
  include_directories(${zmq_INCLUDE_DIRS})
endif ()

if (NOT ZMQ_FOUND)
	BUILD_ERROR ("zmq not found, Please install ...")
endif()

########################################
# Include man pages stuff
include (${project_cmake_dir}/Ronn2Man.cmake)
add_manpage_target()
