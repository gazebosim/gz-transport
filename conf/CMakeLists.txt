include (${project_cmake_dir}/Utils.cmake)

set (confs
  transport.yaml
)

install(FILES ${confs} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
