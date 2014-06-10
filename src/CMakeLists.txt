include (${project_cmake_dir}/Utils.cmake)

set (sources
  Discovery.cc
  DiscoveryPrivate.cc
  Node.cc
  NodePrivate.cc
  TopicStorage.cc
  Packet.cc
)

set (gtest_sources
  Discovery_TEST.cc
  HandlerStorage_TEST.cc
  Node_TEST.cc
  Packet_TEST.cc
  TopicStorage_TEST.cc
)

ign_build_tests(${gtest_sources})
ign_add_library(${PROJECT_NAME_LOWER} ${sources})
target_link_libraries(${PROJECT_NAME_LOWER} zmq uuid protobuf robot_msgs czmq)
link_directories(${ROBOT_MSGS_LIBRARY_DIRS})
ign_install_library(${PROJECT_NAME_LOWER})