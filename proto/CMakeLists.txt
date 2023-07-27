set(proto_files
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/discovery.proto
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/parameter.proto
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/parameter_declaration.proto
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/parameter_declarations.proto
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/parameter_error.proto
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/parameter_name.proto
  ${PROJECT_SOURCE_DIR}/proto/gz/msgs/parameter_value.proto
)

gz_msgs_generate_messages(
  TARGET gz-transport${PROJECT_VERSION_MAJOR}-msgs
  PROTO_PACKAGE "gz.msgs"
  MSGS_PATH ${PROJECT_SOURCE_DIR}/proto
  MSGS_PROTOS ${proto_files}
  DEPENDENCIES gz-msgs${GZ_MSGS_VER}::gz-msgs${GZ_MSGS_VER}-msgs
)

install(
  DIRECTORY gz
  DESTINATION share/protos
  COMPONENT proto
  FILES_MATCHING PATTERN "*.proto")
