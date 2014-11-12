########################################
# Find the Ignition_Transport library
#find_package(ignition-transport QUIET REQUIRED)
#if (NOT ignition-transport_FOUND)
#  message (STATUS "Looking for libignition-transport - not found")
#  BUILD_WARNING ("ignition-transport not found, transport support will be disabled.")
#  set (HAVE_TRANSPORT OFF CACHE BOOL "HAVE TRANSPORT" FORCE)
#else ()
#  message (STATUS "Looking for libignition-transport - found")
#  set (HAVE_TRANSPORT ON CACHE BOOL "HAVE TRANSPORT" FORCE)
#endif ()

include_directories(
  ${CMAKE_BINARY_DIR}/test
)

set (test_sources
  ign_TEST.cc
)

if (IGN_COMMAND)
  ign_build_tests(${test_sources})
endif ()
