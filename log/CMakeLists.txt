set(logging_sources
  src/Log.cc
)

set(log_lib_target ${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}-logging)

add_library(${log_lib_target} SHARED
  ${logging_sources}
)
target_include_directories(${log_lib_target} PUBLIC
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
