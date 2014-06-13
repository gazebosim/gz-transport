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
pkg_check_modules(robot_msgs robot_msgs)
if (NOT robot_msgs_FOUND)
  BUILD_ERROR ("robot_msgs not found.")
else()
  include_directories(${robot_msgs_INCLUDE_DIRS})
  link_directories(${robot_msgs_LIBRARY_DIRS})
endif ()

#################################################
# Find ZeroMQ.
# TODO, which version is the minimun to work with this software
pkg_check_modules(zmq libzmq)

if (NOT zmq_FOUND)
  message (STATUS "Looking for zmq pkgconfig file - not found")
  BUILD_ERROR ("zmq not found, Please install zmq")
else ()
  message (STATUS "Looking for zmq pkgconfig file - found")
  include_directories(${zmq_INCLUDE_DIRS})
endif ()

#################################################
# Find czmq.:
# TODO, which version is the minimun to work with this software
pkg_check_modules(czmq libczmq)

if (NOT czmq_FOUND)
  message (STATUS "Looking for czmq pkgconfig file - not found")
  BUILD_ERROR ("czmq not found, Please install czmq")
else ()
  message (STATUS "Looking for czmq pkgconfig file - found")
  include_directories(${czmq_INCLUDE_DIRS})
endif ()

########################################
# Include man pages stuff
include (${project_cmake_dir}/Ronn2Man.cmake)
add_manpage_target()

#################################################
# Macro to check for visibility capability in compiler
# Original idea from: https://gitorious.org/ferric-cmake-stuff/
macro (check_gcc_visibility)
  include (CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-fvisibility=hidden GCC_SUPPORTS_VISIBILITY)
endmacro()
