# Generate ruby script
get_filename_component(input_script "${CMAKE_CURRENT_SOURCE_DIR}/cmdlog.rb.in" ABSOLUTE)
get_filename_component(output_script "${CMAKE_CURRENT_BINARY_DIR}/cmdlog${PROJECT_MAJOR_VERSION}.rb" ABSOLUTE)
add_custom_command (
  OUTPUT ${output_script}
  DEPENDS ${input_script}
  COMMAND ${CMAKE_COMMAND}
    "-DINPUT_FILE=${input_script}"
    "-DOUTPUT_FILE=${output_script}"
    "-DIGN_LIBRARY_NAME=$<TARGET_FILE:${log_lib_target}>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/generate_time_configure_file.cmake"
)
add_custom_target(generate_ruby_script ALL DEPENDS ${output_script})
install(FILES ${output_script} DESTINATION lib/ruby/ignition)

# Generate command yaml
set(ruby_script_install "${CMAKE_INSTALL_PREFIX}/lib/ruby/ignition/cmdlog${PROJECT_MAJOR_VERSION}")
get_filename_component(input_script "${CMAKE_CURRENT_SOURCE_DIR}/transportlog.yaml.in" ABSOLUTE)
get_filename_component(output_script "${CMAKE_CURRENT_BINARY_DIR}/transportlog${PROJECT_MAJOR_VERSION}.yaml" ABSOLUTE)
add_custom_command (
  OUTPUT ${output_script}
  DEPENDS ${input_script}
  COMMAND ${CMAKE_COMMAND}
    "-DINPUT_FILE=${input_script}"
    "-DOUTPUT_FILE=${output_script}"
    "-DPROJECT_NAME_LOWER=${log_lib_target}"
    "-DIGN_LIBRARY_PATH=${ruby_script_install}"
    "-DPROJECT_VERSION_FULL=${PROJECT_VERSION_FULL}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/generate_time_configure_file.cmake"
)
add_custom_target(generate_cmd_yaml ALL DEPENDS ${output_script})
install(FILES ${output_script} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/ignition/")

ign_build_tests(
  TYPE "UNIT"
  SOURCES
    LogCommandAPI_TEST.cc
  LIB_DEPS ${log_lib_target}
  TEST_LIST logging_tests
)
foreach(test_target ${logging_tests})
  set_tests_properties(${logging_tests} PROPERTIES
    ENVIRONMENT IGN_TRANSPORT_LOG_SQL_PATH=${PROJECT_SOURCE_DIR}/log/sql)
endforeach()
