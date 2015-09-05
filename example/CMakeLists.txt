cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set (examples
  publisher
  subscriber
  responser
  requester
  requester_async
)

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
foreach (EXAMPLE ${examples})
  add_executable(${EXAMPLE} ${EXAMPLE}.cc ${PROTO_SRC})
  target_link_libraries(${EXAMPLE} ${IGNITION-TRANSPORT_LIBRARIES})
  add_dependencies(${EXAMPLE} protobuf_compilation)

  if (MSVC)
    # Copy the ZMQ DLLs.
    add_custom_command(TARGET ${EXAMPLE} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "../${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-3_2_4.dll"
      $<TARGET_FILE_DIR:${EXAMPLE}> VERBATIM)

    add_custom_command(TARGET ${EXAMPLE} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "../${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-gd-3_2_4.dll"
      $<TARGET_FILE_DIR:${EXAMPLE}> VERBATIM)
  endif()
endforeach()
