# Integration tests

ign_build_tests(
  TYPE "INTEGRATION"
  TEST_LIST logging_tests
  SOURCES
    query.cc
  LIB_DEPS
    ${log_lib_target}
  INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/test/
)

foreach(test_target ${logging_tests})
  set_tests_properties(${test_target} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)
  target_compile_definitions(${test_target}
    PRIVATE IGN_TRANSPORT_LOG_SQL_PATH="${PROJECT_SOURCE_DIR}/log/sql")
  target_compile_definitions(${test_target}
    PRIVATE IGN_TRANSPORT_LOG_BUILD_PATH="$<TARGET_FILE_DIR:${test_target}>")

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
