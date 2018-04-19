#===============================================================================
# Generate the ruby script for internal testing.
# Note that the major version of the library is included in the name.
# TODO: We do not currently have tests for the `ign log` command, so this is currently going unused.
set(cmd_log_script_generated_test "${CMAKE_BINARY_DIR}/log/test/lib/ruby/ignition/cmdlog${PROJECT_VERSION_MAJOR}.rb")
set(cmd_log_script_configured_test "${cmd_log_script_generated_test}.configured")

# Set the log_library_location variable to the full path of the library file
# within the build directory
set(log_library_location "$<TARGET_FILE:${log_lib_target}>")

configure_file(
  "cmdlog.rb.in"
  "${cmd_log_script_configured_test}"
  @ONLY)

file(GENERATE
  OUTPUT "${cmd_log_script_generated_test}"
  INPUT  "${cmd_log_script_configured_test}")


#===============================================================================
# Used for the installed version.
# Generate the ruby script that gets installed.
# Note that the major version of the library is included in the name.
set(cmd_log_script_generated "${CMAKE_CURRENT_BINARY_DIR}/cmdlog${PROJECT_VERSION_MAJOR}.rb")
set(cmd_log_script_configured "${cmd_log_script_generated}.configured")

# Set the log_library_location variable to the relative path to the library file
# within the install directory structure.
set(log_library_location "../../../${CMAKE_INSTALL_LIBDIR}/$<TARGET_FILE_NAME:${log_lib_target}>")

configure_file(
  "cmdlog.rb.in"
  "${cmd_log_script_configured}"
  @ONLY)

file(GENERATE
  OUTPUT "${cmd_log_script_generated}"
  INPUT  "${cmd_log_script_configured}")

install(FILES ${cmd_log_script_generated} DESTINATION lib/ruby/ignition)
