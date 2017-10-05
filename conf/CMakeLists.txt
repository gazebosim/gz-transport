# Used only for internal testing.
set(ign_library_path "${CMAKE_BINARY_DIR}/test/lib/ruby/ignition/cmdtransport${PROJECT_VERSION_MAJOR}")

# Generate a configuration file for internal testing.
# Note that the major version of the library is included in the name.
# Ex: transport0.yaml
configure_file(
  "transport.yaml.in"
    "${CMAKE_BINARY_DIR}/test/conf/transport${PROJECT_VERSION_MAJOR}.yaml" @ONLY)

# Used for the installed version.
set(ign_library_path "${CMAKE_INSTALL_PREFIX}/lib/ruby/ignition/cmdtransport${PROJECT_VERSION_MAJOR}")

# Generate the configuration file that is installed.
# Note that the major version of the library is included in the name.
# Ex: transport0.yaml
configure_file(
  "transport.yaml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/transport${PROJECT_VERSION_MAJOR}.yaml" @ONLY)

# Install the yaml configuration files in an unversioned location.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/transport${PROJECT_VERSION_MAJOR}.yaml DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/ignition/)
