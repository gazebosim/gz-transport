# Unit tests
set(logging_tests
  Batch_TEST.cc
  Log_TEST.cc
  Message_TEST.cc
  Recorder_TEST.cc
)
set(TEST_TYPE "UNIT")
ign_build_tests(${logging_tests})
foreach(source_file ${logging_tests})
  string(REGEX REPLACE ".cc" "" BINARY_NAME ${source_file})
  set(BINARY_NAME ${TEST_TYPE}_${BINARY_NAME})
  target_link_libraries(${BINARY_NAME} ${log_lib_target})
  set_tests_properties(${BINARY_NAME} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)
endforeach()
