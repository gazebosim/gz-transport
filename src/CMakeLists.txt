# Collect source files into the "sources" variable and unit test files into the
# "gtest_sources" variable.
ign_get_libsources_and_unittests(sources gtest_sources)

# Create the library target.
ign_add_library(${PROJECT_LIBRARY_TARGET_NAME} ${sources})

# Turn on C++14 support for the library.
ign_set_project_public_cxx_standard(14)

# Link the libraries that we always need.
target_link_libraries(${PROJECT_LIBRARY_TARGET_NAME}
  PUBLIC
    ignition-msgs${IGN_MSGS_VER}::ignition-msgs${IGN_MSGS_VER}
  PRIVATE
    ${ZeroMQ_TARGET}
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

if(MSVC)
  # On Windows, UNIT_Discovery_TEST uses some socket functions and therefore
  # needs to link to the Windows socket library. An easy, maintainable way to
  # do that is to tell it to link to the ZeroMQ target, and let it get the
  # socket library dependency transitively.
  if(TARGET UNIT_Discovery_TEST)
    target_link_libraries(UNIT_Discovery_TEST
      ${ZeroMQ_TARGET})
  endif()
endif()

# Command line support.
add_subdirectory(cmd)
