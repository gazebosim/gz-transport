##################################################
# Hack: to just build the files but not add them into the testsuite
# (like auxiliary apps using gtest files called from other tests)
# set the IGN_SKIP_IN_TESTSUITE variable to true. The variable will
# be set to false at the end of the function.
macro (ign_build_tests)

  # Find the Python interpreter for running the
  # check_test_ran.py script
  find_package(PythonInterp QUIET)

  # Build all the tests
  foreach(GTEST_SOURCE_file ${ARGN})
    string(REGEX REPLACE ".cc" "" BINARY_NAME ${GTEST_SOURCE_file})
    set(BINARY_NAME ${TEST_TYPE}_${BINARY_NAME})
    if(USE_LOW_MEMORY_TESTS)
      add_definitions(-DUSE_LOW_MEMORY_TESTS=1)
    endif(USE_LOW_MEMORY_TESTS)

    set_source_files_properties(${PROTO_SRC} PROPERTIES GENERATED TRUE)

    add_executable(${BINARY_NAME} ${GTEST_SOURCE_file} ${PROTO_SRC})

    add_dependencies(${BINARY_NAME}
      ${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}
      gtest gtest_main
      protobuf_compilation
    )

    if (MSVC)
      # Suppress Protobuf message generation warnings.
      target_compile_options(${BINARY_NAME}
        PUBLIC /wd4018 /wd4100 /wd4127 /wd4244 /wd4267 /wd4512)

      # Suppress the "decorated name length exceed" warning (inside the STL).
      target_compile_options(${BINARY_NAME} PUBLIC "/wd4503")

      # Copy the ZMQ DLLs.
      add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-4_0_4.dll"
        ${CMAKE_CURRENT_BINARY_DIR} VERBATIM)

      add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-gd-4_0_4.dll"
        ${CMAKE_CURRENT_BINARY_DIR} VERBATIM)
    endif()

    target_link_libraries(${BINARY_NAME}
      ${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}
    )

    if (UNIX)
      target_link_libraries(${BINARY_NAME}
        libgtest.a
        libgtest_main.a
        pthread
      )
    elseif(WIN32)
      target_link_libraries(${BINARY_NAME}
        gtest
        gtest_main
      )
    endif()

    if (NOT DEFINED IGN_SKIP_IN_TESTSUITE)
      set(IGN_SKIP_IN_TESTSUITE False)
    endif()

    if (NOT IGN_SKIP_IN_TESTSUITE)
      add_test(${BINARY_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test_results/${BINARY_NAME}.xml)

      set_tests_properties(${BINARY_NAME} PROPERTIES TIMEOUT 240)

      if (PYTHONINTERP_FOUND)
        # Check that the test produced a result and create a failure if
        # it didn't. Guards against crashed and timed out tests.
        add_test(check_${BINARY_NAME} python
          ${PROJECT_SOURCE_DIR}/tools/check_test_ran.py
          ${CMAKE_BINARY_DIR}/test_results/${BINARY_NAME}.xml)
      endif()
    endif()
  endforeach()

  # If IGN_SKIP_IN_TESTSUITE was set to True, back to default. This way
  # we request for explicit definition before calling the macro
  if (IGN_SKIP_IN_TESTSUITE)
    set(IGN_SKIP_IN_TESTSUITE False)
  endif()
endmacro()
