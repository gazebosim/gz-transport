set(logging_sources
  src/Log.cc
)

set(log_lib_target ${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}-log)

add_library(${log_lib_target} SHARED
  ${logging_sources}
)
target_include_directories(${log_lib_target} PUBLIC
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

set_target_properties(${log_lib_target} PROPERTIES SOVERSION ${PROJECT_MAJOR_VERSION} VERSION ${PROJECT_VERSION_FULL})
install(TARGETS ${log_lib_target} DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT shlib)
