set(TEST_TYPE "INTEGRATION")

set(tests
  authPubSub.cc
  scopedTopic.cc
  twoProcessesPubSub.cc
  twoProcessesSrvCall.cc
  twoProcessesSrvCallStress.cc
  twoProcessesSrvCallSync1.cc
  twoProcessesSrvCallWithoutInput.cc
  twoProcessesSrvCallWithoutInputStress.cc
  twoProcessesSrvCallWithoutInputSync1.cc
  twoProcessesSrvCallWithoutOutput.cc
  twoProcessesSrvCallWithoutOutputStress.cc
)

ign_build_tests(TYPE INTEGRATION SOURCES ${tests})

set(auxiliary_files
  authPubSubSubscriberInvalid_aux
  fastPub_aux
  pub_aux
  pub_aux_throttled
  scopedTopicSubscriber_aux
  twoProcessesPublisher_aux
  twoProcessesPubSubSubscriber_aux
  twoProcessesSrvCallReplier_aux
  twoProcessesSrvCallReplierIncreasing_aux
  twoProcessesSrvCallWithoutInputReplier_aux
  twoProcessesSrvCallWithoutInputReplierIncreasing_aux
  twoProcessesSrvCallWithoutOutputReplier_aux
  twoProcessesSrvCallWithoutOutputReplierIncreasing_aux
)

# Build the auxiliary files.
foreach(AUX_EXECUTABLE ${auxiliary_files})
  ign_add_executable(INTEGRATION_${AUX_EXECUTABLE} ${AUX_EXECUTABLE}.cc)

  # Include the interface directories that we always need.
  ign_target_interface_include_directories(INTEGRATION_${AUX_EXECUTABLE}
    ${PROJECT_LIBRARY_TARGET_NAME})

  # Link the libraries that we always need.
  target_link_libraries(INTEGRATION_${AUX_EXECUTABLE}
    PRIVATE
      ${PROJECT_LIBRARY_TARGET_NAME}
      gtest
  )

  if(UNIX)
    # pthread is only available on Unix machines
    target_link_libraries(INTEGRATION_${AUX_EXECUTABLE}
      PRIVATE pthread)
  endif()

endforeach(AUX_EXECUTABLE)
