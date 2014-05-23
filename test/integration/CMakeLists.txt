set(TEST_TYPE "INTEGRATION")

set(tests
#	beaconTest.cc
	twoProcessesPubSub.cc
)

link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})
