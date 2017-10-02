cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

#============================================================================
# Find ignition-cmake
#============================================================================
# If you get an error at this line, you need to install ignition-cmake
find_package(ignition-cmake0 REQUIRED)

#============================================================================
# Set up the project
#============================================================================
ign_configure_project(transport 4.0.0)

#============================================================================
# Set project-specific options
#============================================================================

# ignition-transport currently has no options that are unique to it

#============================================================================
# Search for project-specific dependencies
#============================================================================
message(STATUS "\n\n-- ====== Finding Dependencies ======")

#--------------------------------------
# Find Protobuf
ign_find_package(Protobuf
                 VERSION 2.3.0
                 PKGCONFIG_VER_COMPARISON >=
                 REQUIRED)

#--------------------------------------
# Find ZeroMQ
ign_find_package(ZeroMQ
                 VERSION 4.1.0
                 PKGCONFIG_VER_COMPARISON >=
                 REQUIRED)

#--------------------------------------
# Find cppzmq
ign_find_package(CPPZMQ
                 REQUIRED)

#--------------------------------------
# Find uuid
ign_find_package(UUID
                 REQUIRED)

#--------------------------------------
# Find ignition-msgs
set(IGN_MSGS_VER 0)
ign_find_package(ignition-msgs${IGN_MSGS_VER}
                 REQUIRED)

#--------------------------------------
# Find ignition-tools
ign_find_package(ignition-tools
                 QUIET)

#============================================================================
# Configure the build
#============================================================================
ign_configure_build(QUIT_IF_BUILD_ERRORS)

#============================================================================
# ign command line support
#============================================================================
add_subdirectory(conf)

#============================================================================
# Test the command line commands
#============================================================================
add_subdirectory(tools)

#============================================================================
# Create package information
#============================================================================
ign_create_packages()
