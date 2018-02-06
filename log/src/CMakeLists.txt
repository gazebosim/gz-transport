# Unit tests
set(TEST_TYPE "UNIT")
ign_build_tests(
  TYPE "UNIT"
  SOURCES
    Descriptor_TEST.cc
    Log_TEST.cc
  LIB_DEPS ${log_lib_target}
  TEST_LIST logging_tests
)
foreach(test_target ${logging_tests})
  set_tests_properties(${logging_tests} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)
endforeach()
