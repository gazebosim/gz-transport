find_package(ignition-common0 REQUIRED)

set(log_lib_target ${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}-log)

add_subdirectory(src/cmd)

set(logging_sources
  src/cmd/LogCommandAPI.cc
  src/Log.cc
)

add_library(${log_lib_target} SHARED
  ${logging_sources}
)

include(GenerateExportHeader)
generate_export_header(${log_lib_target}
  EXPORT_MACRO_NAME "IGNITION_TRANSPORT_LOG_VISIBLE"
  NO_EXPORT_MACRO_NAME "IGNITION_TRANSPORT_LOG_HIDDEN"
  EXPORT_FILE_NAME "include/ignition/transport/log/Export.hh"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ignition/transport/log/Export.hh"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ignition/transport/log/")

target_include_directories(${log_lib_target} PUBLIC
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_link_libraries(${log_lib_target} PRIVATE
  ${ignition-common0_LIBRARIES}
)

set_target_properties(${log_lib_target} PROPERTIES SOVERSION ${PROJECT_MAJOR_VERSION} VERSION ${PROJECT_VERSION_FULL})
install(TARGETS ${log_lib_target} DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT shlib)
