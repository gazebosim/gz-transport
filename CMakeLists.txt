cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

#============================================================================
# Initialize the project
#============================================================================
project(gz-transport VERSION 15.0.0)

#============================================================================
# Find gz-cmake
#============================================================================
# If you get an error at this line, you need to install gz-cmake
find_package(gz-cmake REQUIRED)

#============================================================================
# Configure the project
#============================================================================
set(skip_pybind11_default_value OFF)
option(SKIP_PYBIND11
      "Skip generating Python bindings via pybind11"
      ${skip_pybind11_default_value})

# Python interfaces vars
include(CMakeDependentOption)
cmake_dependent_option(USE_SYSTEM_PATHS_FOR_PYTHON_INSTALLATION
      "Install python modules in standard system paths in the system"
      OFF "NOT SKIP_PYBIND11" OFF)

cmake_dependent_option(USE_DIST_PACKAGES_FOR_PYTHON
      "Use dist-packages instead of site-package to install python modules"
      OFF "NOT SKIP_PYBIND11" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

gz_configure_project(VERSION_SUFFIX)

#============================================================================
# Set project-specific options
#============================================================================

# gz-transport currently has no options that are unique to it

if (UNIX AND NOT APPLE)
  set (EXTRA_TEST_LIB_DEPS stdc++fs)
else()
  set (EXTRA_TEST_LIB_DEPS)
endif()

#============================================================================
# Search for project-specific dependencies
#============================================================================
message(STATUS "\n\n-- ====== Finding Dependencies ======")

#--------------------------------------
# Python interfaces
if (SKIP_PYBIND11)
  message(STATUS "SKIP_PYBIND11 set - disabling python bindings")
  find_package(Python3 COMPONENTS Interpreter)
else()
  find_package(Python3
    COMPONENTS Interpreter
    OPTIONAL_COMPONENTS Development)
endif()

#--------------------------------------
# Find Protobuf
gz_find_package(GzProtobuf
                REQUIRED
                PRETTY Protobuf)

#--------------------------------------
# Find ZeroMQ
gz_find_package(ZeroMQ VERSION 4 REQUIRED PRIVATE)
list(APPEND GZ_TRANSPORT_IMPLEMENTATION_LIST "zeromq")

if (UNIX AND NOT APPLE)
  execute_process(COMMAND lsb_release -cs
    OUTPUT_VARIABLE RELEASE_CODENAME
    RESULT_VARIABLE LSB_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if( NOT (${LSB_RESULT} STREQUAL "0"))
    message(WARNING "lsb_release executable not found. Disabling focal-specific workarounds")
  elseif (${RELEASE_CODENAME} STREQUAL "focal")
    set(UBUNTU_FOCAL 1)
  elseif (${RELEASE_CODENAME} STREQUAL "bullseye")
    set(DEBIAN_BULLSEYE 1)
  endif()
endif()

#--------------------------------------
# Find cppzmq
gz_find_package(CPPZMQ REQUIRED PRIVATE
  PKGCONFIG_IGNORE # NOTE: cppzmq does not seem to offer a pkg-config file
  PRETTY CppZMQ)

########################################
# Find Zenoh
option(GZ_TRANSPORT_ENABLE_ZENOH "Enable gz-transport zenoh implementation" ON)

if (GZ_TRANSPORT_ENABLE_ZENOH)
  gz_find_package(zenohc PRIVATE QUIET)
  gz_find_package(zenohcxx PRIVATE QUIET)

  # If both zenohc and zenohcpp packages are not found then
  # look for these pacakages installed by ROS 2 zenoh_cpp_vendor package
  if (NOT zenohc_FOUND AND NOT zenohcxx_FOUND)
    # Set CMAKE_PREFIX_PATH in order to find ROS 2 packages from plain cmake
    # package, see https://github.com/ament/ament_cmake/issues/304
    set(ros2_zenoh_cmake_path "/opt/ros/$ENV{ROS_DISTRO}/opt/zenoh_cpp_vendor/lib/cmake")
    if (EXISTS ${ros2_zenoh_cmake_path})
      list(PREPEND CMAKE_PREFIX_PATH ${ros2_zenoh_cmake_path})
      gz_find_package(zenohc PRIVATE QUIET)
      gz_find_package(zenohcxx PRIVATE QUIET)
    endif()
  endif ()

  if (NOT zenohcxx_FOUND)
    message (STATUS "Looking for zenohcxx - not found")
    set (HAVE_ZENOH OFF CACHE BOOL "HAVE ZENOH" FORCE)
  elseif (NOT zenohc_FOUND)
    message (STATUS "Looking for zenohc - not found")
    set (HAVE_ZENOH OFF CACHE BOOL "HAVE ZENOH" FORCE)
  else ()
    message (STATUS "Looking for zenohcxx and zenohc - found")
    set (HAVE_ZENOH ON CACHE BOOL "HAVE ZENOH" FORCE)
    list(APPEND EXTRA_TEST_LIB_DEPS "zenohcxx::zenohc")
    list(APPEND GZ_TRANSPORT_IMPLEMENTATION_LIST "zenoh")
  endif ()
endif()

#--------------------------------------
# Find uuid
if (MSVC)
  set(skip_pybind11_default_value ON)
  message (STATUS "UUID: Using Windows RPC UuidCreate function\n")
else()
  gz_find_package(UUID REQUIRED)
endif()

#--------------------------------------
# Find gz-utils
gz_find_package(gz-utils REQUIRED COMPONENTS cli)

#--------------------------------------
# Find gz-msgs
gz_find_package(gz-msgs REQUIRED)

#--------------------------------------
# Find ifaddrs
gz_find_package(IFADDRS QUIET)
if (IFADDRS_FOUND)
  set (HAVE_IFADDRS ON CACHE BOOL "HAVE IFADDRS" FORCE)
else ()
  set (HAVE_IFADDRS OFF CACHE BOOL "HAVE IFADDRS" FORCE)
endif()

#--------------------------------------
# Find if command is available. This is used to enable tests.
# Note that CLI files are installed regardless of whether the dependency is
# available during build time
find_program(HAVE_GZ_TOOLS gz)
set (GZ_TOOLS_VER 2)

#--------------------------------------
# Find SQLite3
gz_find_package(SQLite3
  VERSION 3.7.13
  REQUIRED_BY log
  PRIVATE_FOR log
  PRETTY sqlite3)

#--------------------------------------
# Check GZ_TRANSPORT_DEFAULT_IMPLEMENTATION value
set(GZ_TRANSPORT_DEFAULT_IMPLEMENTATION "zeromq" CACHE
    STRING "Set default gz-transport implementation")
if (NOT ${GZ_TRANSPORT_DEFAULT_IMPLEMENTATION} IN_LIST GZ_TRANSPORT_IMPLEMENTATION_LIST)
  message(FATAL_ERROR "Invalid GZ_TRANSPORT_DEFAULT_IMPLEMENTATION value: ${GZ_TRANSPORT_DEFAULT_IMPLEMENTATION}")
endif()

#============================================================================
# Configure the build
#============================================================================
gz_configure_build(QUIT_IF_BUILD_ERRORS
  COMPONENTS log parameters)

#============================================================================
# gz command line support
#============================================================================
add_subdirectory(conf)

#============================================================================
# gz transport python bindings
#============================================================================
if (NOT SKIP_PYBIND11)
  if (Python3_Development_FOUND)
    add_subdirectory(python)
  else ()
    message(WARNING "Python development libraries are missing: Python interfaces are disabled.")
  endif()
endif()

#============================================================================
# Create package information
#============================================================================
gz_create_packages()

#============================================================================
# Configure documentation
#============================================================================
configure_file(${CMAKE_SOURCE_DIR}/api.md.in ${CMAKE_BINARY_DIR}/api.md)
configure_file(${CMAKE_SOURCE_DIR}/tutorials.md.in ${CMAKE_BINARY_DIR}/tutorials.md)

gz_create_docs(
  API_MAINPAGE_MD "${CMAKE_BINARY_DIR}/api.md"
  TUTORIALS_MAINPAGE_MD "${CMAKE_BINARY_DIR}/tutorials.md")
