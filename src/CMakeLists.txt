include (${project_cmake_dir}/Utils.cmake)

set (sources
  AdvertiseOptions.cc
  Discovery.cc
  Helpers.cc
  ign.cc
  NetUtils.cc
  Node.cc
  NodeOptions.cc
  NodeShared.cc
  Packet.cc
  Publisher.cc
  TopicUtils.cc
  Uuid.cc
)

set (gtest_sources
  AdvertiseOptions_TEST.cc
  Discovery_TEST.cc
  Helpers_TEST.cc
  HandlerStorage_TEST.cc
  NetUtils_TEST.cc
  Node_TEST.cc
  NodeOptions_TEST.cc
  Packet_TEST.cc
  Publisher_TEST.cc
  TopicStorage_TEST.cc
  TopicUtils_TEST.cc
  Uuid_TEST.cc
)

set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER} PROPERTIES
                            GENERATED TRUE)
include_directories(${CMAKE_BINARY_DIR}/)
include_directories(${CMAKE_BINARY_DIR}/test/)

ign_build_tests(${gtest_sources})

if (UNIX)
  ign_add_library(${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION} ${sources})
elseif(WIN32)
  # Probotuf compilation is recommended and configured for a STATIC .lib
  # Not mixing static and shared and just build ignition transport as STATIC
  # on Windows
  add_library(${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION} STATIC ${sources})
endif()

# Protobuf module does not have LIBRARIES but LIBRARY
message("ZeroMQ_LIBRARIES: ${ZeroMQ_LIBRARIES}")
target_link_libraries(${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}
                      ${ZeroMQ_LIBRARIES}
                      ${uuid_LIBRARIES})

if(WIN32)
  target_link_libraries(${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION} ws2_32 Iphlpapi
			optimized
			    ${PROTOBUF_LIBRARY}
			    ${PROTOBUF_PROTOC_LIBRARY}
			debug
			    ${PROTOBUF_LIBRARY_DEBUG}
			    ${PROTOBUF_PROTOC_LIBRARY_DEBUG})
else()
  target_link_libraries(${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}
                        ${PROTOBUF_LIBRARY})
endif()

ign_install_library(${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION})

add_subdirectory(cmd)
