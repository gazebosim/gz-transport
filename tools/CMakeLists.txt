include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/test
)

set (test_sources
  ign_TEST.cc
)

if (IGNITION-TOOLS_BINARY_DIRS)
  ign_build_tests(TYPE UNIT SOURCES ${test_sources}
    TEST_LIST test_list
    LIB_DEPS stdc++fs)

  foreach(test ${test_list})

    # Inform each test of its output directory so it knows where to call the
    # auxiliary files from. Using a generator expression here is useful for
    # multi-configuration generators, like Visual Studio.
    target_compile_definitions(${test} PRIVATE
      "DETAIL_IGN_TRANSPORT_TEST_DIR=\"$<TARGET_FILE_DIR:${test}>\""
      "IGN_TEST_LIBRARY_PATH=\"$<TARGET_FILE_DIR:${PROJECT_LIBRARY_TARGET_NAME}>\"")

  endforeach()

endif ()
