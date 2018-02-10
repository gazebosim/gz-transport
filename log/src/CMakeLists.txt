# Unit tests
set(TEST_TYPE "UNIT")
ign_build_tests(
  TYPE "UNIT"
  SOURCES
    Batch_TEST.cc
    Descriptor_TEST.cc
    Log_TEST.cc
    Message_TEST.cc
    QualifiedTime_TEST.cc
    QueryOptions_TEST.cc
    Recorder_TEST.cc
    SqlStatement_TEST.cc
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
