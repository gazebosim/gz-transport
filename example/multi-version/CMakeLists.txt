cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)

project(ignition-transport-examples)

# transport 6 + msgs 3
find_package(ignition-transport6 QUIET REQUIRED OPTIONAL_COMPONENTS log)

# transport 7 + msgs 4
find_package(ignition-transport7 QUIET REQUIRED OPTIONAL_COMPONENTS log)

# Message generation. Only required when using custom Protobuf messages.
find_package(Protobuf REQUIRED)

add_executable(publisher6 publisher6.cc)
target_link_libraries(publisher6 ignition-transport6::core)

add_executable(subscriber6 subscriber6.cc)
target_link_libraries(subscriber6 ignition-transport6::core)

add_executable(publisher7 publisher7.cc)
target_link_libraries(publisher7 ignition-transport7::core)

add_executable(subscriber7 subscriber7.cc)
target_link_libraries(subscriber7 ignition-transport7::core)
