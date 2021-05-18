# Used only for internal testing.
set(ign_library_path "${CMAKE_BINARY_DIR}/test/lib/$<CONFIG>/ruby/ignition/cmd${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}")

# Generate a configuration file for internal testing.
# Note that the major version of the library is included in the name.
# Ex: transport0.yaml
configure_file(
  "${IGN_DESIGNATION}.yaml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.yaml.configured" @ONLY)

file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/test/conf/$<CONFIG>/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.yaml"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.yaml.configured")

# Used for the installed version.
set(ign_library_path "${CMAKE_INSTALL_PREFIX}/lib/ruby/ignition/cmd${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}")

# Generate the configuration file that is installed.
# Note that the major version of the library is included in the name.
# Ex: transport0.yaml
configure_file(
  "${IGN_DESIGNATION}.yaml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.yaml" @ONLY)

# Install the yaml configuration files in an unversioned location.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.yaml
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/ignition/)
