set(TEST_TYPE "INTEGRATION")

set(tests
  authPubSub.cc
  scopedTopic.cc
  callback_scope_TEST.cc
  statistics.cc
  twoProcsPubSub.cc
  twoProcsSrvCall.cc
  twoProcsSrvCallStress.cc
  twoProcsSrvCallSync1.cc
  twoProcsSrvCallWithoutInput.cc
  twoProcsSrvCallWithoutInputStress.cc
  twoProcsSrvCallWithoutInputSync1.cc
  twoProcsSrvCallWithoutOutput.cc
  twoProcsSrvCallWithoutOutputStress.cc
)

# Test symbols having the right name on linux only.
if (UNIX AND NOT APPLE)
  configure_file(all_symbols_have_version.bash.in
    ${CMAKE_CURRENT_BINARY_DIR}/all_symbols_have_version.bash @ONLY)
  add_test(NAME INTEGRATION_versioned_symbols
    COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/all_symbols_have_version.bash $<TARGET_FILE:${PROJECT_LIBRARY_TARGET_NAME}>)
endif()

gz_build_tests(TYPE INTEGRATION SOURCES ${tests}
  TEST_LIST test_list
  LIB_DEPS ${EXTRA_TEST_LIB_DEPS})

foreach(test ${test_list})
  # Inform each test of its output directory so it knows where to call the
  # auxiliary files from. Using a generator expression here is useful for
  # multi-configuration generators, like Visual Studio.
  target_compile_definitions(${test} PRIVATE
    "DETAIL_GZ_TRANSPORT_TEST_DIR=\"$<TARGET_FILE_DIR:${test}>\"")
endforeach()

set(auxiliary_files
  authPubSubSubscriberInvalid_aux
  fastPub_aux
  pub_aux
  pub_aux_throttled
  scopedTopicSubscriber_aux
  twoProcsPublisher_aux
  twoProcsPubSubSubscriber_aux
  twoProcsSrvCallReplier_aux
  twoProcsSrvCallReplierInc_aux
  twoProcsSrvCallWithoutInputReplier_aux
  twoProcsSrvCallWithoutInputReplierInc_aux
  twoProcsSrvCallWithoutOutputReplier_aux
  twoProcsSrvCallWithoutOutputReplierInc_aux
)

# Build the auxiliary files.
foreach(AUX_EXECUTABLE ${auxiliary_files})
  gz_add_executable(${AUX_EXECUTABLE} ${AUX_EXECUTABLE}.cc)

  # Link the libraries that we always need.
  target_link_libraries(${AUX_EXECUTABLE}
    PRIVATE
      ${PROJECT_LIBRARY_TARGET_NAME}
      gtest
      ${EXTRA_TEST_LIB_DEPS}
  )
endforeach(AUX_EXECUTABLE)


foreach(test ${test_list})
  target_compile_definitions(${test} PRIVATE
    "AUTH_PUB_SUB_SUBSCRIBER_INVALID_EXE=\"$<TARGET_FILE:authPubSubSubscriberInvalid_aux>\""
    "FAST_PUB_EXE=\"$<TARGET_FILE:fastPub_aux>\""
    "PUB_EXE=\"$<TARGET_FILE:pub_aux>\""
    "PUB_THROTTLED_EXE=\"$<TARGET_FILE:pub_aux_throttled>\""
    "SCOPED_TOPIC_SUBSCRIBER_EXE=\"$<TARGET_FILE:scopedTopicSubscriber_aux>\""

    "TWO_PROCS_PUBLISHER_EXE=\"$<TARGET_FILE:twoProcsPublisher_aux>\""
    "TWO_PROCS_PUB_SUB_SUBSCRIBER_EXE=\"$<TARGET_FILE:twoProcsPubSubSubscriber_aux>\""

    "TWO_PROCS_SRV_CALL_REPLIER_EXE=\"$<TARGET_FILE:twoProcsSrvCallReplier_aux>\""
    "TWO_PROCS_SRV_CALL_REPLIER_INC_EXE=\"$<TARGET_FILE:twoProcsSrvCallReplierInc_aux>\""
    "TWO_PROCS_SRV_CALL_WITHOUT_INPUT_REPLIER_EXE=\"$<TARGET_FILE:twoProcsSrvCallWithoutInputReplier_aux>\""
    "TWO_PROCS_SRV_CALL_WITHOUT_INPUT_REPLIER_INC_EXE=\"$<TARGET_FILE:twoProcsSrvCallWithoutInputReplierInc_aux>\""
    "TWO_PROCS_SRV_CALL_WITHOUT_OUTPUT_REPLIER_EXE=\"$<TARGET_FILE:twoProcsSrvCallWithoutOutputReplier_aux>\""
    "TWO_PROCS_SRV_CALL_WITHOUT_OUTPUT_REPLIER_INC_EXE=\"$<TARGET_FILE:twoProcsSrvCallWithoutOutputReplierInc_aux>\"")
endforeach()
