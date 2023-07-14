# Integration tests

gz_build_tests(
  TYPE "INTEGRATION"
  TEST_LIST logging_tests
  SOURCES
    recorder.cc
    playback.cc
    query.cc
  LIB_DEPS
    ${PROJECT_LIBRARY_TARGET_NAME}-log
    ${EXTRA_TEST_LIB_DEPS}
  INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/test/
)

# Test symbols having the right name on linux only.
if (UNIX AND NOT APPLE)
  add_test(NAME INTEGRATION_log_versioned_symbols
    COMMAND bash ${PROJECT_BINARY_DIR}/test/integration/all_symbols_have_version.bash $<TARGET_FILE:${PROJECT_LIBRARY_TARGET_NAME}-log>)
endif()

foreach(test_target ${logging_tests})

  set_tests_properties(${test_target} PROPERTIES
    ENVIRONMENT GZ_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)
  target_compile_definitions(${test_target}
    PRIVATE GZ_TRANSPORT_LOG_SQL_PATH="${PROJECT_SOURCE_DIR}/log/sql")
  target_compile_definitions(${test_target}
    PRIVATE GZ_TRANSPORT_LOG_BUILD_PATH="$<TARGET_FILE_DIR:${test_target}>")

endforeach()

set (aux
  topicChirp_aux.cc
)

foreach(source_file ${aux})
  string(REGEX REPLACE ".cc" "" AUX_EXECUTABLE ${source_file})
  set(BINARY_NAME ${TEST_TYPE}_${AUX_EXECUTABLE})

  gz_add_executable(${BINARY_NAME} ${AUX_EXECUTABLE}.cc)

  # Include the interface directories that we always need.
  gz_target_interface_include_directories(${BINARY_NAME}
    ${PROJECT_LIBRARY_TARGET_NAME})

  # Link the libraries that we always need.
  target_link_libraries(${BINARY_NAME}
    PRIVATE
      ${PROJECT_LIBRARY_TARGET_NAME}
      ${log_lib_target}
      gtest
      ${EXTRA_TEST_LIB_DEPS}
  )

  if(UNIX)
    # pthread is only available on Unix machines
    target_link_libraries(${BINARY_NAME}
      PRIVATE pthread)
  endif()

  target_compile_definitions(${BINARY_NAME}
    PRIVATE GZ_TRANSPORT_LOG_BUILD_PATH="$<TARGET_FILE_DIR:${BINARY_NAME}>")
endforeach()

# gz log CLI test
if (HAVE_GZ_TOOLS)
  set(GZ_CONFIG_PATH "${CMAKE_BINARY_DIR}/log/test/lib/ruby/gz")

  add_test(gz_log_record_no_overwrite
    ruby ${CMAKE_CURRENT_SOURCE_DIR}/gz_log_record_no_overwrite.rb
  )

  # Test disabled
  # add_test(gz_log_record_force
  #   ruby ${CMAKE_CURRENT_SOURCE_DIR}/gz_log_record_force.rb
  # )

  set_tests_properties(
    gz_log_record_no_overwrite
    # Test disabled
    # gz_log_record_force
    PROPERTIES
    ENVIRONMENT
      "GZ_CONFIG_PATH=${GZ_CONFIG_PATH};GZ_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql"
    TIMEOUT 10
  )
endif ()
