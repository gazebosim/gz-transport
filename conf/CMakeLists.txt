include (${project_cmake_dir}/Utils.cmake)

# Generate a configuration file.
# Note that the major version of the library is included in the name.
# Ex: transport0.yaml
configure_file(
  "transport.yaml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/transport${PROJECT_MAJOR_VERSION}.yaml" @ONLY)


# Install the yaml configuration files in an unversioned location.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/transport${PROJECT_MAJOR_VERSION}.yaml DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/ignition/)