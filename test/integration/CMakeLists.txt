set(TEST_TYPE "INTEGRATION")

set(tests
  scopedTopic.cc
	twoProcessesPubSub.cc
  twoProcessesSrvCall.cc
  twoProcessesSrvCallStress.cc
  twoProcessesSrvCallSync1.cc
  twoProcessesSrvCallSync2.cc
)

include_directories(SYSTEM ${CMAKE_BINARY_DIR}/test/)
link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})
