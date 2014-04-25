include (${project_cmake_dir}/Utils.cmake)

set (sources
  Node.cc
  NodePrivate.cc
  NetUtils.cc
  Packet.cc
  TopicsInfo.cc
  socket.cc
)

set (gtest_sources
  Node_TEST.cc
  Packet_TEST.cc
  TopicsInfo_TEST.cc
)

ign_build_tests(${gtest_sources})
ign_add_library(${PROJECT_NAME_LOWER} ${sources})
target_link_libraries(${PROJECT_NAME_LOWER} zmq uuid protobuf ${ROBOT_MSGS_LIBRARIES})
link_directories(${ROBOT_MSGS_LIBRARY_DIRS})
ign_install_library(${PROJECT_NAME_LOWER})
