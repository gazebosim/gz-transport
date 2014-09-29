cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition-transport QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION-TRANSPORT_CXX_FLAGS}")

include_directories(${IGNITION-TRANSPORT_INCLUDE_DIRS})
link_directories(${IGNITION-TRANSPORT_LIBRARY_DIRS})

add_subdirectory(msg)

set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER} PROPERTIES
                            GENERATED TRUE)
include_directories(${CMAKE_BINARY_DIR})

add_executable(publisher publisher.cc ${PROTO_SRC})
target_link_libraries(publisher ${IGNITION-TRANSPORT_LIBRARIES})
add_dependencies(publisher protobuf_compilation)

add_executable(subscriber subscriber.cc ${PROTO_SRC})
target_link_libraries(subscriber ${IGNITION-TRANSPORT_LIBRARIES})
add_dependencies(subscriber protobuf_compilation)

add_executable(responser responser.cc ${PROTO_SRC})
target_link_libraries(responser ${IGNITION-TRANSPORT_LIBRARIES})
add_dependencies(responser protobuf_compilation)

add_executable(requester requester.cc ${PROTO_SRC})
target_link_libraries(requester ${IGNITION-TRANSPORT_LIBRARIES})
add_dependencies(requester protobuf_compilation)

add_executable(requester_async requester_async.cc ${PROTO_SRC})
target_link_libraries(requester_async ${IGNITION-TRANSPORT_LIBRARIES})
add_dependencies(requester_async protobuf_compilation)

add_executable(basic_requester basic_requester.cc ${PROTO_SRC})
target_link_libraries(basic_requester ${IGNITION-TRANSPORT_LIBRARIES} zmq)
add_dependencies(basic_requester protobuf_compilation)

add_executable(basic_responser basic_responser.cc ${PROTO_SRC})
target_link_libraries(basic_responser ${IGNITION-TRANSPORT_LIBRARIES} zmq)
add_dependencies(basic_responser protobuf_compilation)