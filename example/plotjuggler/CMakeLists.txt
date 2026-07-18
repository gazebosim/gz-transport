cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(gz_pj_bridge)

set(CMAKE_CXX_STANDARD 17)

# Find Packages
find_package(gz-cmake REQUIRED)
find_package(gz-transport REQUIRED)
find_package(gz-msgs REQUIRED)
find_package(Protobuf REQUIRED)
gz_find_package(ZeroMQ VERSION 4 REQUIRED PRIVATE)

add_executable(gz_pj_bridge gz_pj_bridge.cc)

target_link_libraries(gz_pj_bridge
  gz-transport::core
  gz-msgs::core
  protobuf::libprotobuf
  CPPZMQ::CPPZMQ
  ${ZeroMQ_TARGET}
)
