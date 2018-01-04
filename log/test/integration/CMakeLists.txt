# Integration tests
set(tests
  recorder.cc
)

set(aux
  topicChirp_aux.cc
)

include_directories(SYSTEM ${CMAKE_BINARY_DIR}/test/)

set(TEST_TYPE "INTEGRATION")
ign_build_tests(${tests})

set(IGN_SKIP_IN_TESTSUITE True)
ign_build_tests(${aux})

foreach(source_file ${tests})
  string(REGEX REPLACE ".cc" "" BINARY_NAME ${source_file})
  set(BINARY_NAME ${TEST_TYPE}_${BINARY_NAME})

  set_tests_properties(${BINARY_NAME} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)

  target_compile_definitions(${BINARY_NAME}
    PRIVATE IGN_TRANSPORT_LOG_SQL_PATH="${PROJECT_SOURCE_DIR}/log/sql")

endforeach()

foreach(source_file ${tests} ${aux})
  string(REGEX REPLACE ".cc" "" BINARY_NAME ${source_file})
  set(BINARY_NAME ${TEST_TYPE}_${BINARY_NAME})

  target_link_libraries(${BINARY_NAME} ${log_lib_target})

  target_compile_definitions(${BINARY_NAME}
    PRIVATE IGN_TRANSPORT_LOG_BUILD_PATH="$<TARGET_FILE_DIR:${BINARY_NAME}>")

endforeach()
