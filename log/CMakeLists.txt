find_package(PkgConfig REQUIRED)
pkg_search_module(SQLite3 REQUIRED sqlite3>=3.6.19)

set(log_lib_target ${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}-log)

set(logging_sources
  src/Batch.cc
  src/Console.cc
  src/Descriptor.cc
  src/Log.cc
  src/Message.cc
  src/MsgIter.cc
  src/raii-sqlite3.cc
  src/SqlStatement.cc
)

add_library(${log_lib_target} SHARED
  ${logging_sources}
)

# Generate visibility macros
include(GenerateExportHeader)
generate_export_header(${log_lib_target}
  EXPORT_MACRO_NAME "IGNITION_TRANSPORT_LOG_VISIBLE"
  NO_EXPORT_MACRO_NAME "IGNITION_TRANSPORT_LOG_HIDDEN"
  EXPORT_FILE_NAME "include/ignition/transport/log/Export.hh"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ignition/transport/log/Export.hh"
  DESTINATION "${IGN_INCLUDE_INSTALL_DIR_FULL}/log")

# Include directories
target_include_directories(${log_lib_target} PUBLIC
  $<INSTALL_INTERFACE:${IGN_INCLUDE_INSTALL_DIR_FULL}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_include_directories(${log_lib_target} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${SQLite3_INCLUDE_DIRS}
)

# Library dependencies
target_link_libraries(${log_lib_target} PUBLIC
  ${PROJECT_NAME_LOWER}
)
target_link_libraries(${log_lib_target} PRIVATE
  ${SQLite3_LIBRARIES}
)

# Compile flags
target_compile_options(${log_lib_target} PRIVATE
  ${SQLite3_CFLAGS}
)

# ign_build_tests macro doesn't support building tests in a subfolder
add_subdirectory(src)

# Configure file with build time info
set(SCHEMA_INSTALL_BASE ${CMAKE_INSTALL_FULL_DATAROOTDIR}/ignition/${PROJECT_NAME_LOWER})
install(DIRECTORY sql DESTINATION ${SCHEMA_INSTALL_BASE})
set(SCHEMA_INSTALL_PATH ${SCHEMA_INSTALL_BASE}/sql)
configure_file(src/build_config.hh.in build_config.hh @ONLY)

# Library installation
set_target_properties(${log_lib_target} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION_FULL})
install(TARGETS ${log_lib_target} DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT shlib)

# Header installation
install(DIRECTORY include/ignition DESTINATION ${IGN_INCLUDE_INSTALL_DIR})
