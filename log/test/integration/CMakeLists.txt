# Integration tests


add_library(ChirpParams STATIC ./ChirpParams.cc)
target_link_libraries(ChirpParams PUBLIC ${PROJECT_LIBRARY_TARGET_NAME}-log ${EXTRA_TEST_LIB_DEPS})
target_compile_definitions(ChirpParams
  PRIVATE TOPIC_CHIRP_EXE="$<TARGET_FILE:topicChirp_aux>")

gz_add_executable(topicChirp_aux topicChirp_aux.cc)
target_link_libraries(topicChirp_aux ChirpParams)

gz_build_tests(
  TYPE "INTEGRATION"
  TEST_LIST logging_tests
  SOURCES
    recorder.cc
    playback.cc
    query.cc
  LIB_DEPS
    ChirpParams
    ${PROJECT_LIBRARY_TARGET_NAME}-log
    ${EXTRA_TEST_LIB_DEPS}
    test_config
  INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/test/
)

# Test symbols having the right name on linux only.
if (UNIX AND NOT APPLE)
  add_test(NAME INTEGRATION_log_versioned_symbols
    COMMAND bash ${PROJECT_BINARY_DIR}/test/integration/all_symbols_have_version.bash $<TARGET_FILE:${PROJECT_LIBRARY_TARGET_NAME}-log>)
endif()

foreach(test_target ${logging_tests})
  set(_env_vars)
  list(APPEND _env_vars "GZ_IP=127.0.0.1")
  list(APPEND _env_vars "GZ_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql")
  set_tests_properties(${test_target} PROPERTIES
    ENVIRONMENT "${_env_vars}")
  target_compile_definitions(${test_target}
    PRIVATE GZ_TRANSPORT_LOG_SQL_PATH="${PROJECT_SOURCE_DIR}/log/sql")
endforeach()
