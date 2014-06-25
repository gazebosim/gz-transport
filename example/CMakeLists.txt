cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition_transport QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION_TRANSPORT_CXX_FLAGS}")
include_directories(${IGNITION_TRANSPORT_INCLUDE_DIRS})
link_directories(${IGNITION_TRANSPORT_LIBRARY_DIRS})

add_executable(publisher publisher.cc)
target_link_libraries(publisher ${IGNITION_TRANSPORT_LIBRARIES})

add_executable(subscriber subscriber.cc)
target_link_libraries(subscriber ${IGNITION_TRANSPORT_LIBRARIES})
