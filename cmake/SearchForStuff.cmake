include (${project_cmake_dir}/Utils.cmake)
include (CheckCXXSourceCompiles)

include (${project_cmake_dir}/FindOS.cmake)
include (FindPkgConfig)

# It is know that raring compiler 4.7.3 is not able to compile the software
# Check for a fully valid c++11 compiler
if (CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION LESS 4.8)
      message(STATUS "Not found a compatible c++11 gcc compiler")
      BUILD_ERROR("GCC version is lower than 4.8. Need a compatible c++11 compiler")
  endif()
endif()

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
pkg_check_modules(zmq libzmq>=3.2.0)

if (NOT zmq_FOUND)
  message (STATUS "Looking for zmq pkgconfig file - not found")
  BUILD_ERROR ("zmq not found, Please install zmq")
else ()
  message (STATUS "Looking for zmq pkgconfig file - found")
  include_directories(${zmq_INCLUDE_DIRS})
  link_directories(${zmq_LIBRARY_DIRS})
endif ()

#################################################
# Find cppzeromq header (shipped together with zeromq in debian/ubuntu but
# different upstream projects and tarballs)
find_path(cppzmq_INCLUDE_DIRS zmq.hpp PATHS ${zmq_INCLUDE_DIRS})
if (NOT cppzmq_INCLUDE_DIRS)
  message(STATUS "cppzmq header file was not found")
  BUILD_ERROR("cppzmq header file was not found")
else()
  message(STATUS "cppzmq file - found")
  include_directories(${cppzmq_INCLUDE_DIRS})
endif()

#################################################
# Find czmq.:
pkg_check_modules(czmq libczmq>=2.1.0)

if (NOT czmq_FOUND)
  message (STATUS "Looking for czmq pkgconfig file - not found")
  BUILD_ERROR ("czmq not found, Please install czmq")
else ()
  message (STATUS "Looking for czmq pkgconfig file - found")
  include_directories(${czmq_INCLUDE_DIRS})
  link_directories(${czmq_LIBRARY_DIRS})
endif ()

#################################################
# Find uuid:
pkg_check_modules(uuid uuid)

if (NOT uuid_FOUND)
  message (STATUS "Looking for uuid pkgconfig file - not found")
  BUILD_ERROR ("uuid not found, Please install uuid")
else ()
  message (STATUS "Looking for uuid pkgconfig file - found")
  include_directories(${uuid_INCLUDE_DIRS})
  link_directories(${uuid_LIBRARY_DIRS})
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
