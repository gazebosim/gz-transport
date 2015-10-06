cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition-transport0 QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION-TRANSPORT_CXX_FLAGS}")
include_directories(${IGNITION-TRANSPORT_INCLUDE_DIRS})
link_directories(${IGNITION-TRANSPORT_LIBRARY_DIRS})

# Message generation
add_subdirectory(msgs)
set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER}
                            PROPERTIES GENERATED TRUE)
include_directories(${CMAKE_BINARY_DIR})

# Generate examples
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

if (MSVC)
  # Copy the ZMQ DLLs.
  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-3_2_4.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-gd-3_2_4.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)
endif()
