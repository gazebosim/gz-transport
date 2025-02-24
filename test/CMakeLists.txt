include_directories (
  ${PROJECT_SOURCE_DIR}/test/gtest/include
  ${PROJECT_SOURCE_DIR}/test/gtest
  ${PROJECT_SOURCE_DIR}/test
  ${PROJECT_BINARY_DIR}
)

configure_file (test_config.h.in ${PROJECT_BINARY_DIR}/include/ignition/transport/test_config.h)
configure_file (test_config.h.in ${PROJECT_BINARY_DIR}/include/gz/transport/test_config.h)

# Build gtest
add_library(gtest STATIC gtest/src/gtest-all.cc)
add_library(gtest_main STATIC gtest/src/gtest_main.cc)
target_link_libraries(gtest_main gtest)
set(GTEST_LIBRARY "${PROJECT_BINARY_DIR}/test/libgtest.a")
set(GTEST_MAIN_LIBRARY "${PROJECT_BINARY_DIR}/test/libgtest_main.a")

execute_process(COMMAND cmake -E remove_directory ${CMAKE_BINARY_DIR}/test_results)
execute_process(COMMAND cmake -E make_directory ${CMAKE_BINARY_DIR}/test_results)
include_directories(${GTEST_INCLUDE_DIRS})

add_subdirectory(integration)
add_subdirectory(performance)
add_subdirectory(regression)
<<<<<<< HEAD
=======

configure_file (test_config.hh.in ${PROJECT_BINARY_DIR}/include/test_config.hh)
add_library(test_config INTERFACE)
target_include_directories(test_config INTERFACE
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/test
)
target_compile_definitions(test_config INTERFACE
# Location of the "gz" command
  "GZ_EXE=\"${HAVE_GZ_TOOLS}\""
  "TRANSPORT_BASH_COMPLETION_SH=\"${PROJECT_SOURCE_DIR}/src/cmd/transport.bash_completion.sh\""
# Auxiliary executables for test
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
>>>>>>> 72758e8 (Code cleanup (#578))
