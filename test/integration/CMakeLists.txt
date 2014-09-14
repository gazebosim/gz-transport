set(TEST_TYPE "INTEGRATION")

set(tests
	beaconTest.cc
  scopedTopic.cc
	twoProcessesPubSub.cc
  twoProcessesSrvCall.cc
  twoProcessesSrvCallSync1.cc
  twoProcessesSrvCallSync2.cc
)

include_directories(SYSTEM ${CMAKE_BINARY_DIR}/test/msg)
link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})
