# Turn on C++11 support for the library.
set(CMAKE_CXX_STANDARD 11)

# Collect source files into the "sources" variable and unit test files into the
# "gtest_sources" variable.
ign_get_libsources_and_unittests(sources gtest_sources)

# Create the library target.
ign_add_library(${PROJECT_LIBRARY_TARGET_NAME} ${sources})

# Link the libraries that we always need.
target_link_libraries(${PROJECT_LIBRARY_TARGET_NAME}
  PUBLIC
    ignition-msgs${IGN_MSGS_VER}::ignition-msgs${IGN_MSGS_VER}
  PRIVATE
    ZeroMQ::ZeroMQ
    CPPZMQ::CPPZMQ
)

# Windows system library provides UUID
if (NOT MSVC)
  target_link_libraries(${PROJECT_LIBRARY_TARGET_NAME}
    PUBLIC
      UUID::UUID
  )
endif()

# Create installation instructions for the library target. This must be called
# in the same scope that the target is created.
ign_install_library()

# Build the unit tests.
ign_build_tests(TYPE UNIT SOURCES ${gtest_sources})

# Command line support.
add_subdirectory(cmd)
