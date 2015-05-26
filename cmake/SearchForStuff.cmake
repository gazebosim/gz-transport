include (${project_cmake_dir}/Utils.cmake)
include (CheckCXXSourceCompiles)

include (${project_cmake_dir}/FindOS.cmake)

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

include_directories(${PROTOBUF_INCLUDE_DIR})

#################################################
# Find ZeroMQ.
include (${project_cmake_dir}/FindZeroMQ.cmake)

if (NOT ZeroMQ_FOUND)
  BUILD_ERROR ("zmq not found, Please install zmq")
else ()
  include_directories(${ZeroMQ_INCLUDE_DIRS})
  link_directories(${ZeroMQ_LIBRARY_DIRS})
endif ()

#################################################
# Find cppzeromq header (shipped together with zeromq in debian/ubuntu but
# different upstream projects and tarballs)
#
# Provide the PATH using CPPZMQ_HEADER_PATH
#
find_path(cppzmq_INCLUDE_DIRS
          zmq.hpp
	  PATHS
	   ${zmq_INCLUDE_DIRS}
	   ${CPPZMQ_HEADER_PATH})

if (NOT cppzmq_INCLUDE_DIRS)
  message(STATUS "cppzmq header file was not found")
  BUILD_ERROR("cppzmq header file was not found")
else()
  message(STATUS "cppzmq file - found")
  include_directories(${cppzmq_INCLUDE_DIRS})
endif()

#################################################
# Find uuid
#  - In UNIX we use uuid library
#  - In Windows the native RPC call, no dependency needed
if (UNIX)
  include (FindPkgConfig REQUIRED)
  pkg_check_modules(uuid uuid)

  if (NOT uuid_FOUND)
    message (STATUS "Looking for uuid pkgconfig file - not found")
    BUILD_ERROR ("uuid not found, Please install uuid")
  else ()
    message (STATUS "Looking for uuid pkgconfig file - found")
    include_directories(${uuid_INCLUDE_DIRS})
    link_directories(${uuid_LIBRARY_DIRS})
  endif ()
elseif (MSVC)
  message (STATUS "Using Windows RPC UuidCreate function")
endif()

#################################################
# Find ifaddrs.h
find_path(HAVE_IFADDRS ifaddrs.h)
if (HAVE_IFADDRS)
  message (STATUS "ifaddrs.h found.")
  set (HAVE_IFADDRS ON CACHE BOOL "HAVE IFADDRS" FORCE)
else ()
  BUILD_WARNING ("ifaddrs.h not found.")
  set (HAVE_IFADDRS OFF CACHE BOOL "HAVE IFADDRS" FORCE)
endif()

#################################################
# Find ign command line utility:
find_package(ignition-tools)
if (IGNITION-TOOLS_BINARY_DIRS)
  set (HAVE_IGN TRUE)
else()
  BUILD_WARNING ("ignition-tools not found, for command line utilities, please install ignition-tools.")
endif()

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
