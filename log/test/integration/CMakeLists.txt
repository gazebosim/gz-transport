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

message("Got test list ${logging_tests}")

foreach(test_target ${logging_tests})
  set_tests_properties(${test_target} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)
  target_compile_definitions(${test_target}
    PRIVATE IGN_TRANSPORT_LOG_SQL_PATH="${PROJECT_SOURCE_DIR}/log/sql")
  target_compile_definitions(${test_target}
    PRIVATE IGN_TRANSPORT_LOG_BUILD_PATH="$<TARGET_FILE_DIR:${test_target}>")
endforeach()
