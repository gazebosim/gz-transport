# Integration tests

ign_build_tests(
  TYPE "INTEGRATION"
  TEST_LIST logging_tests
  SOURCES
    recorder.cc
    playback.cc
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

set (aux
  topicChirp_aux.cc
)

foreach(source_file ${tests} ${aux})
  string(REGEX REPLACE ".cc" "" AUX_EXECUTABLE ${source_file})
  set(BINARY_NAME ${TEST_TYPE}_${BINARY_NAME})

  ign_add_executable(${BINARY_NAME} ${AUX_EXECUTABLE}.cc)

  # Include the interface directories that we always need.
  ign_target_interface_include_directories(${BINARY_NAME}
    ${PROJECT_LIBRARY_TARGET_NAME})

  # Link the libraries that we always need.
  target_link_libraries(${BINARY_NAME}
    PRIVATE
      ${PROJECT_LIBRARY_TARGET_NAME}
      ${log_lib_target}
      gtest
  )

  if(UNIX)
    # pthread is only available on Unix machines
    target_link_libraries(${BINARY_NAME}
      PRIVATE pthread)
  endif()

  target_compile_definitions(${BINARY_NAME}
    PRIVATE IGN_TRANSPORT_LOG_BUILD_PATH="$<TARGET_FILE_DIR:${BINARY_NAME}>")
endforeach()


