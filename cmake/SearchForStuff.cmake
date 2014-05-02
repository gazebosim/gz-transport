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

########################################
# robot_msgs used for testing
find_package(robot_msgs REQUIRED)
if (NOT robot_msgs_FOUND)
  BUILD_ERROR ("Missing: Robot msgs (librobot_msgs)")
else ()
 	message (STATUS "robot_msgs include: " ${ROBOT_MSGS_INCLUDE_DIRS})
 	include_directories(${ROBOT_MSGS_INCLUDE_DIRS})
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

#################################################
# Find czmq.
find_path (czmq_INCLUDE_DIRS czmq.h)
set (CZMQ_FOUND True)

find_library(CZMQ_LIBRARY
    NAMES czmq
    HINTS "${CMAKE_PREFIX_PATH}/curl/lib"
)

if (NOT czmq_INCLUDE_DIRS)
  message (STATUS "Looking for czmq.h - not found")
  set (CZMQ_FOUND False)
else ()
  message (STATUS "Looking for czmq.h - found")
  include_directories(${czmq_INCLUDE_DIRS})
endif ()

if (NOT CZMQ_FOUND)
  BUILD_ERROR ("czmq not found, Please install ...")
endif()

########################################
# Include man pages stuff
include (${project_cmake_dir}/Ronn2Man.cmake)
add_manpage_target()
