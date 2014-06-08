set(TEST_TYPE "INTEGRATION")

set(tests
  scopedTopic.cc
	twoProcessesPubSub.cc
  twoProcessesSrvCall.cc
)

link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})
