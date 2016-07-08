cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition-transport2 QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION-TRANSPORT_CXX_FLAGS}")
include_directories(${IGNITION-TRANSPORT_INCLUDE_DIRS})
link_directories(${IGNITION-TRANSPORT_LIBRARY_DIRS})

# Generate examples
if (EXISTS "${CMAKE_SOURCE_DIR}/publisher.cc")
  add_executable(publisher publisher.cc)
  target_link_libraries(publisher ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/subscriber.cc")
  add_executable(subscriber subscriber.cc)
  target_link_libraries(subscriber ${IGNITION-TRANSPORT_LIBRARIES})
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

if (EXISTS "${CMAKE_SOURCE_DIR}/ResponserOneways.cc")
  add_executable(ResponserOneway ResponserOneway.cc
  target_link_libraries(ResponserOneway ${IGNITION-TRANSPORT_LIBRARIES})
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/RequesterOneway.cc")
  add_executable(RequesterOneway RequesterOneway.cc
  target_link_libraries(RequesterOneway ${IGNITION-TRANSPORT_LIBRARIES})
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
    "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-3_2_4.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ZeroMQ_ROOT_DIR}/bin/libzmq-v120-mt-gd-3_2_4.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

  # Copy the Ignition Messages DLL.
  add_custom_command(TARGET publisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${IGNITION-MSGS_FOLDER}/lib/ignition-msgs0.dll"
    $<TARGET_FILE_DIR:publisher> VERBATIM)

endif()
