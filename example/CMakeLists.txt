cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition_transport QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION_TRANSPORT_CXX_FLAGS}")

# Find the Ignition_Msgs library
find_package(ignition_msgs QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION_MSGS_CXX_FLAGS}")

include_directories(${IGNITION_TRANSPORT_INCLUDE_DIRS}
  ${IGNITION_MSGS_INCLUDE_DIRS})
link_directories(${IGNITION_TRANSPORT_LIBRARY_DIRS}
  ${IGNITION_MSGS_LIBRARY_DIRS})

add_executable(publisher publisher.cc)
target_link_libraries(publisher ${IGNITION_TRANSPORT_LIBRARIES}
  ${IGNITION_MSGS_LIBRARIES})

add_executable(subscriber subscriber.cc)
target_link_libraries(subscriber ${IGNITION_TRANSPORT_LIBRARIES}
  ${IGNITION_MSGS_LIBRARIES})

add_executable(responser responser.cc)
target_link_libraries(responser ${IGNITION_TRANSPORT_LIBRARIES}
  ${IGNITION_MSGS_LIBRARIES})

add_executable(requester requester.cc)
target_link_libraries(requester ${IGNITION_TRANSPORT_LIBRARIES}
  ${IGNITION_MSGS_LIBRARIES})

add_executable(requester_async requester_async.cc)
target_link_libraries(requester_async ${IGNITION_TRANSPORT_LIBRARIES}
  ${IGNITION_MSGS_LIBRARIES})
