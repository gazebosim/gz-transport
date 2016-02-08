set(TEST_TYPE "INTEGRATION")

set(tests
  scopedTopic.cc
  twoProcsPubSub.cc
  twoProcsSrv.cc
  twoProcsSrvStress.cc
  twoProcsSrvSync1.cc
)

include_directories(SYSTEM ${CMAKE_BINARY_DIR}/test/)
link_directories(${PROJECT_BINARY_DIR}/test)

ign_build_tests(${tests})

# Skip auxiliary files in the test suite
set(IGN_SKIP_IN_TESTSUITE True)

set(auxiliary_files
  scopedTopicSub_aux.cc
  twoProcsPub_aux.cc
  twoProcsSub_aux.cc
  twoProcsSrvRep_aux.cc
  twoProcsSrvRepInc_aux.cc
)

ign_build_tests(${auxiliary_files})
