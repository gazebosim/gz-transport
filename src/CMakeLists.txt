include (${project_cmake_dir}/Utils.cmake)

set (sources
  Discovery.cc
  #ign.cc
  NetUtils.cc
  #Node.cc
  NodeShared.cc
  Packet.cc
  Publisher.cc
  TopicUtils.cc
  Uuid.cc
)

set (gtest_sources
  Discovery_TEST.cc
  HandlerStorage_TEST.cc
  #Node_TEST.cc
  Packet_TEST.cc
  TopicStorage_TEST.cc
  TopicUtils_TEST.cc
  Uuid_TEST.cc
)

set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER} PROPERTIES
                            GENERATED TRUE)
include_directories(${CMAKE_BINARY_DIR}/test/)

ign_build_tests(${gtest_sources})

if (UNIX)
  ign_add_library(${PROJECT_NAME_LOWER} ${sources})
elseif(WIN32)
  # Probotuf compilation is recommended and configured for a STATIC .lib
  # Not mixing static and shared and just build ignition transport as STATIC
  # on Windows
  add_library(${PROJECT_NAME_LOWER} STATIC ${sources})
endif()

# Protobuf module does not have LIBRARIES but LIBRARY
message("ZeroMQ_LIBRARIES: ${ZeroMQ_LIBRARIES}")
target_link_libraries(${PROJECT_NAME_LOWER}
                      ${ZeroMQ_LIBRARIES}
                      ${uuid_LIBRARIES})

if(WIN32)
  target_link_libraries(${PROJECT_NAME_LOWER} ws2_32 Iphlpapi)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME_LOWER} ${PROTOBUF_LIBRARY_DEBUG})
  else()
    target_link_libraries(${PROJECT_NAME_LOWER} ${PROTOBUF_LIBRARY})
  endif()
else()
  target_link_libraries(${PROJECT_NAME_LOWER} ${PROTOBUF_LIBRARY})
endif()

ign_install_library(${PROJECT_NAME_LOWER})

add_dependencies(${PROJECT_NAME_LOWER} protobuf_compilation)

add_subdirectory(cmd)
