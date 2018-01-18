cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition-transport4 QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION-TRANSPORT_CXX_FLAGS}")
include_directories(${IGNITION-TRANSPORT_INCLUDE_DIRS})
link_directories(${IGNITION-TRANSPORT_LIBRARY_DIRS})

# Message generation. Only required when using custom Protobuf messages.
find_package(Protobuf REQUIRED)
add_subdirectory(msgs)
set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER}
                            PROPERTIES GENERATED TRUE)
include_directories(${CMAKE_BINARY_DIR})

# Generate examples
if (EXISTS "${CMAKE_SOURCE_DIR}/bench.cc")
  ########################################
  # GFlags
  find_library(gflags_LIBRARIES NAMES gflags)
  find_path(gflags_INCLUDE_DIRS gflags/gflags.h ENV CPATH)
  if (NOT gflags_LIBRARIES OR NOT gflags_INCLUDE_DIRS)
    message (STATUS "Unable to compile 'bench.cc', missing GFLags library")
  else()
    include_directories(SYSTEM ${gflags_INCLUDE_DIRS})
    add_executable(bench bench.cc)
    target_link_libraries(bench ${IGNITION-TRANSPORT_LIBRARIES}
                          ${gflags_LIBRARIES})
  endif()
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/publisher.cc")
  add_executable(publisher publisher.cc)
  target_link_libraries(publisher ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/subscriber.cc")
  add_executable(subscriber subscriber.cc)
  target_link_libraries(subscriber ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/subscriber_generic.cc")
  add_executable(subscriber_generic subscriber_generic.cc)
  target_link_libraries(subscriber_generic ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/responser.cc")
  add_executable(responser responser.cc)
  target_link_libraries(responser ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/requester.cc")
  add_executable(requester requester.cc)
  target_link_libraries(requester ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/requester_async.cc")
  add_executable(requester_async requester_async.cc)
  target_link_libraries(requester_async ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/responser_oneway.cc")
  add_executable(responser_oneway responser_oneway.cc)
  target_link_libraries(responser_oneway ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/requester_oneway.cc")
  add_executable(requester_oneway requester_oneway.cc)
  target_link_libraries(requester_oneway ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/responser_no_input.cc")
  add_executable(responser_no_input responser_no_input.cc)
  target_link_libraries(responser_no_input ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/requester_no_input.cc")
  add_executable(requester_no_input requester_no_input.cc)
  target_link_libraries(requester_no_input ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/requester_async_no_input.cc")
  add_executable(requester_async_no_input requester_async_no_input.cc)
  target_link_libraries(requester_async_no_input ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/publisher_custom_msg.cc")
  add_executable(publisher_custom_msg publisher_custom_msg.cc)
  target_link_libraries(publisher_custom_msg
     ${IGNITION-TRANSPORT_LIBRARIES}
     ${PROTO_SRC}
  )
  add_dependencies(publisher_custom_msg protobuf_compilation)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/subscriber_custom_msg.cc")
  add_executable(subscriber_custom_msg subscriber_custom_msg.cc)
  target_link_libraries(subscriber_custom_msg
    ${IGNITION-TRANSPORT_LIBRARIES}
    ${PROTO_SRC}
  )
  add_dependencies(subscriber_custom_msg protobuf_compilation)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/publisher_raw.cc")
  add_executable(publisher_raw publisher_raw.cc)
  target_link_libraries(publisher_raw ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/subscriber_raw.cc")
  add_executable(subscriber_raw subscriber_raw.cc)
  target_link_libraries(subscriber_raw ${IGNITION-TRANSPORT_LIBRARIES})
endif()


if (MSVC)
  # Suppress Protobuf message generation warnings.
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4100 /wd4127 /wd4244 /wd4267 /wd4512")

  # Suppress the "decorated name length exceed" warning (inside the STL).
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503")

  set(BUILD_SHARED_LIBS FALSE)
  add_definitions(-DBUILDING_STATIC_LIBS -DWIN32_LEAN_AND_MEAN)

  # Don't pull in the Windows min/max macros
  add_definitions(-DNOMINMAX)

  # Copy the ZMQ DLLs.
  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-4_0_4.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-gd-4_0_4.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

  # Copy the Ignition Messages DLL.
  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${IGNITION-MSGS_FOLDER}/lib/ignition-msgs0.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

endif()
