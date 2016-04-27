set(TEST_TYPE "INTEGRATION")

set(tests
  scopedTopic.cc
  twoProcessesPubSub.cc
  twoProcessesSrvCall.cc
  twoProcessesSrvCallStress.cc
  twoProcessesSrvCallSync1.cc
)

include_directories(SYSTEM ${CMAKE_BINARY_DIR}/test/)
link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})

# Skip auxiliary files in the test suite
set(IGN_SKIP_IN_TESTSUITE True)

set(auxiliary_files
  fastPub_aux.cc
  scopedTopicSubscriber_aux.cc
  twoProcessesPublisher_aux.cc
  twoProcessesPubSubSubscriber_aux.cc
  twoProcessesSrvCallReplier_aux.cc
  twoProcessesSrvCallReplierIncreasing_aux.cc
)

ign_build_tests(${auxiliary_files})
