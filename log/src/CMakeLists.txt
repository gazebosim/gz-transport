
ign_get_libsources_and_unittests(sources gtest_sources)

ign_add_component(log SOURCES ${sources} GET_TARGET_NAME log_lib_target)

target_link_libraries(${log_lib_target}
  PRIVATE SQLite3::SQLite3)

# Unit tests
ign_build_tests(
  TYPE "UNIT"
  SOURCES ${gtest_sources}
  LIB_DEPS ${log_lib_target}
  TEST_LIST logging_tests
)

foreach(test_target ${logging_tests})
  set_tests_properties(${logging_tests} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)

  # Gross hack so that the tests can work on Windows. We need a better system for
  # accessing the runtime libraries.
  if(MSVC)

    set(dll_original "$<TARGET_FILE:${log_lib_target}>")
    set(dll_target "$<TARGET_FILE_DIR:${test_target}>/$<TARGET_FILE_NAME:${log_lib_target}>")

    add_custom_command(
      TARGET ${test_target}
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy_if_different ${dll_original} ${dll_target}
      VERBATIM)

  endif()

endforeach()

if(MSVC)
  # The Recorder class uses Discovery, which is a header-only class
  # that makes calls to the native Windows socket library when compiled
  # on Windows. The easiest way to link to the correct socket library is
  # to link to the ZeroMQ target and then get the socket library
  # dependency transitively.
  target_link_libraries(${log_lib_target} PRIVATE
    ${ZeroMQ_TARGET})
endif()

add_subdirectory(cmd)

# Set install instructions for the sql schema, and configure the build to use it
set(SCHEMA_INSTALL_BASE ${CMAKE_INSTALL_DATAROOTDIR}/ignition/${PROJECT_NAME_LOWER})
install(DIRECTORY ../sql DESTINATION ${SCHEMA_INSTALL_BASE})
set(SCHEMA_INSTALL_PATH ${SCHEMA_INSTALL_BASE}/sql)
configure_file(build_config.hh.in build_config.hh @ONLY)


message(STATUS "CMAKE_CURRENT_SOURCE_DIR:${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(${log_lib_target}
  PUBLIC
    # Add this component's include directory to the build interface include
    # directories so the headers are visible while building.
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
  PRIVATE
    # Add the current binary directory as a private include directory while building
    # the logging library. This allows the logging library to see build_config.hh
    # while being built.
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
