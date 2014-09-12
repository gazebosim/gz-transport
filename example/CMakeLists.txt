cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition-transport QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION_TRANSPORT_CXX_FLAGS}")

# Find the Ignition_Msgs library
find_package(ignition-msgs QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION-MSGS_CXX_FLAGS}")

include_directories(${IGNITION-TRANSPORT_INCLUDE_DIRS}
  ${IGNITION-MSGS_INCLUDE_DIRS})
link_directories(${IGNITION-TRANSPORT_LIBRARY_DIRS}
  ${IGNITION-MSGS_LIBRARY_DIRS})

add_executable(publisher publisher.cc)
target_link_libraries(publisher ${IGNITION-TRANSPORT_LIBRARIES}
  ${IGNITION-MSGS_LIBRARIES})

add_executable(subscriber subscriber.cc)
target_link_libraries(subscriber ${IGNITION-TRANSPORT_LIBRARIES}
  ${IGNITION-MSGS_LIBRARIES})

add_executable(responser responser.cc)
target_link_libraries(responser ${IGNITION-TRANSPORT_LIBRARIES}
  ${IGNITION-MSGS_LIBRARIES})

add_executable(requester requester.cc)
target_link_libraries(requester ${IGNITION-TRANSPORT_LIBRARIES}
  ${IGNITION-MSGS_LIBRARIES})

add_executable(requester_async requester_async.cc)
target_link_libraries(requester_async ${IGNITION-TRANSPORT_LIBRARIES}
  ${IGNITION-MSGS_LIBRARIES})
