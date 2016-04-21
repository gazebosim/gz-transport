PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER
  int.proto
  vector3d.proto
)

# Variables needed to propagate through modules
# If more than one layer of cmake use CACHE INTERNAL ...
#set (PROTO_SRCS_VAR ${PROTO_SRCS} CACHE INTERNAL "Sources variable")
set(PROTOBUF_INCLUDE_DIRS ${PROTOBUF_INCLUDE_DIRS} PARENT_SCOPE)
set(PROTOBUF_LIBRARIES ${PROTOBUF_LIBRARIES} PARENT_SCOPE)
set(PROTO_SRC ${PROTO_SRC} PARENT_SCOPE)
set(PROTO_HEADER ${PROTO_HEADER} PARENT_SCOPE)

add_custom_target(protobuf_compilation DEPENDS ${PROTO_SRC})
