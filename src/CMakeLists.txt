include (${project_cmake_dir}/Utils.cmake)

set (sources
  discZmq.cc
  netUtils.cc
  packet.cc
  topicsInfo.cc
  socket.cc
)

set (gtest_sources
  discZmq_TEST.cc
  packet_TEST.cc
  topicsInfo_TEST.cc
)

#################################################
### Do not change the following

ign_build_tests(${gtest_sources})
ign_add_library(${PROJECT_NAME_LOWER} ${sources})
target_link_libraries(${PROJECT_NAME_LOWER} ${Boost_LIBRARIES} zmq uuid protobuf)
ign_install_library(${PROJECT_NAME_LOWER})
