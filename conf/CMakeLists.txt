include (${project_cmake_dir}/Utils.cmake)

set (confs
  transport.yaml
)

# Install the yaml configuration files in an unversioned location.
install(FILES ${confs} DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/ignition/)