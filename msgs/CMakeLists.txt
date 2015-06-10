set(msgs
  int.proto
  vector3d.proto
)

add_executable(ignmsgs_out generator/IgnGenerator.cc generator/ign_generator.cc)

target_link_libraries(ignmsgs_out ${PROTOBUF_LIBRARY} ${PROTOBUF_PROTOC_LIBRARY})

if (UNIX)
  target_link_libraries(ignmsgs_out pthread)
endif()


# Need to add .exe to executable in windows
if (WIN32)
   set(ignmsgs_out_binary "ignmsgs_out.exe")
else()
   set(ignmsgs_out_binary "ignmsgs_out")
endif()

set(PROTO_SRCS)
set(PROTO_HDRS)
foreach(FIL ${msgs})
  get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
  get_filename_component(FIL_WE ${FIL} NAME_WE)

  list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
  list(APPEND PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
      "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
    COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --plugin=protoc-gen-ignmsgs=${CMAKE_CURRENT_BINARY_DIR}/${ignmsgs_out_binary} --cpp_out ${CMAKE_CURRENT_BINARY_DIR} --ignmsgs_out ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL}
    DEPENDS ${ABS_FIL} ignmsgs_out
    COMMENT "Running C++ protocol buffer compiler on ${FIL}"
    VERBATIM )
endforeach()

set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)

ign_add_static_library(${PROJECT_MSGS_NAME} ${PROTO_SRCS})
target_link_libraries(${PROJECT_MSGS_NAME} ${PROTOBUF_LIBRARY} ${PROTOBUF_PROTOC_LIBRARY})
ign_install_library(${PROJECT_MSGS_NAME})
add_dependencies(${PROJECT_MSGS_NAME} ignmsgs_out)

# Install the generated protobuf messages.
install(FILES ${PROTO_HDRS} DESTINATION
  ${INCLUDE_INSTALL_DIR}/${IGN_PROJECT_NAME}/msgs)

