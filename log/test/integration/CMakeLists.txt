# Integration tests

ign_build_tests(
  TYPE "INTEGRATION"
  TEST_LIST logging_tests
  SOURCES
    recorder.cc
    playback.cc
    query.cc
  LIB_DEPS
    ${PROJECT_LIBRARY_TARGET_NAME}-log
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

set (aux
  topicChirp_aux.cc
)

foreach(source_file ${aux})
  string(REGEX REPLACE ".cc" "" AUX_EXECUTABLE ${source_file})
  set(BINARY_NAME ${TEST_TYPE}_${AUX_EXECUTABLE})

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
