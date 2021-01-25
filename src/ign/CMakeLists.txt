add_executable(
  transport 
  transport_main.cc
  ign.cc
)

target_link_libraries(transport
  ignition-utils${IGN_UTILS_VER}::cli
  ${PROJECT_LIBRARY_TARGET_NAME}
)

install(TARGETS transport DESTINATION lib/ignition/transport9)
