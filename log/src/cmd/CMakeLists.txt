# Create log executable
set(log_executable gz-transport-log-main)
add_executable(${log_executable} log_main.cc LogCommandAPI.cc ../Console.cc)
target_link_libraries(${log_executable}
  gz-utils::gz-utils
  ${PROJECT_LIBRARY_TARGET_NAME}
  ${log_lib_target}
)

install(
  TARGETS ${log_executable}
  DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/gz/${GZ_DESIGNATION}${PROJECT_VERSION_MAJOR}/
)

if(BUILD_TESTING)
  gz_build_tests(TYPE UNIT
    SOURCES
      LogCommandAPI_TEST.cc
    LIB_DEPS
      gz-utils::gz-utils
      ${PROJECT_LIBRARY_TARGET_NAME}
    ENVIRONMENT
        GZ_SIM_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
  )
  target_compile_definitions(UNIT_LogCommandAPI_TEST
    PRIVATE
    "GZ_PATH=\"${HAVE_GZ_TOOLS}\""
  )
endif()

#===============================================================================
# Generate the ruby script for internal testing.
# Note that the major version of the library is included in the name.
# TODO(anyone): We do not currently have tests for the `gz log` command, so this is currently going unused.
set(gz_log_ruby_path
  "${CMAKE_BINARY_DIR}/test/lib/$<CONFIG>/ruby/gz/cmdlog${PROJECT_VERSION_MAJOR}")
set(cmd_log_script_generated_test "${gz_log_ruby_path}.rb")
set(cmd_log_script_configured_test
  "${CMAKE_CURRENT_BINARY_DIR}/test_cmdlog${PROJECT_VERSION_MAJOR}.rb.configured")

# Set the log_library_location variable to the full path of the library file
# within the build directory
set(log_exe_location "$<TARGET_FILE:${log_executable}>")

configure_file(
  "cmdlog.rb.in"
  "${cmd_log_script_configured_test}"
  @ONLY
)

file(GENERATE
  OUTPUT "${cmd_log_script_generated_test}"
  INPUT  "${cmd_log_script_configured_test}"
)

configure_file(
  "transportlog.yaml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/transportlog${PROJECT_VERSION_MAJOR}.yaml.configured"
)

file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/test/conf/$<CONFIG>/transportlog${PROJECT_VERSION_MAJOR}.yaml"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/transportlog${PROJECT_VERSION_MAJOR}.yaml.configured"
)

#===============================================================================
# Used for the installed version.
# Generate the ruby script that gets installed.
# Note that the major version of the library is included in the name.
set(cmd_log_script_generated "${CMAKE_CURRENT_BINARY_DIR}/cmdlog${PROJECT_VERSION_MAJOR}.rb")
set(cmd_log_script_configured "${cmd_log_script_generated}.configured")

# Set the log_library_location variable to the relative path to the library file
# within the install directory structure.
set(log_exe_location "../../../${CMAKE_INSTALL_LIBEXECDIR}/gz/${GZ_DESIGNATION}${PROJECT_VERSION_MAJOR}/$<TARGET_FILE_NAME:${log_executable}>")

configure_file(
  "cmdlog.rb.in"
  "${cmd_log_script_configured}"
  @ONLY)

file(GENERATE
  OUTPUT "${cmd_log_script_generated}"
  INPUT  "${cmd_log_script_configured}")

install(FILES ${cmd_log_script_generated} DESTINATION lib/ruby/gz)

# Used for the installed version.
set(gz_log_ruby_path "${CMAKE_INSTALL_PREFIX}/lib/ruby/gz/cmdlog${PROJECT_VERSION_MAJOR}")

set(transportlog_configured "${CMAKE_CURRENT_BINARY_DIR}/transportlog${PROJECT_VERSION_MAJOR}.yaml")
configure_file(
  "transportlog.yaml.in"
  ${transportlog_configured})

install(FILES ${transportlog_configured} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/gz/)
