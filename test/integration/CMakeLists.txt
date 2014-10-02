set(TEST_TYPE "INTEGRATION")

set(tests
  #beaconTest.cc
  scopedTopic.cc
  scopedTopicSubscriber_aux.cc
  twoProcessesPubSub.cc
  twoProcessesPubSubSubscriber_aux.cc
  twoProcessesSrvCall.cc
  twoProcessesSrvCallReplier_aux.cc
  twoProcessesSrvCallSync1.cc
  twoProcessesSrvCallSync2.cc
)

include_directories(SYSTEM ${CMAKE_BINARY_DIR}/test/)
link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})
