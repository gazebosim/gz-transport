# Collect source files into the "sources" variable and unit test files into the
# "gtest_sources" variable.
ign_get_libsources_and_unittests(sources gtest_sources)

# Skip command line tests for Windows, see
# https://github.com/ignitionrobotics/ign-transport/issues/104
if (MSVC)
  list(REMOVE_ITEM gtest_sources ign_TEST.cc)
endif()

# Make a small static lib of command line functions
add_library(ign STATIC ign.cc)
target_link_libraries(ign
  ${PROJECT_LIBRARY_TARGET_NAME}
)

# Build topic CLI executable
set(topic_executable ign-transport-topic)
add_executable(${topic_executable} topic_main.cc)
target_link_libraries(${topic_executable}
  ign
  ignition-utils${IGN_UTILS_VER}::cli
  ${PROJECT_LIBRARY_TARGET_NAME}
)
install(TARGETS ${topic_executable} DESTINATION lib/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/)

# Build service CLI executable
set(service_executable ign-transport-service)
add_executable(${service_executable} service_main.cc)
target_link_libraries(${service_executable}
  ign
  ignition-utils${IGN_UTILS_VER}::cli
  ${PROJECT_LIBRARY_TARGET_NAME}
)
install(TARGETS ${service_executable} DESTINATION lib/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/)

# Build the unit tests.
ign_build_tests(TYPE UNIT SOURCES ${gtest_sources}
  TEST_LIST test_list
  LIB_DEPS ${EXTRA_TEST_LIB_DEPS})

foreach(test ${test_list})
  target_link_libraries(${test} ign)

  # Inform each test of its output directory so it knows where to call the
  # auxiliary files from. Using a generator expression here is useful for
  # multi-configuration generators, like Visual Studio.
  target_compile_definitions(${test} PRIVATE
    "DETAIL_IGN_TRANSPORT_TEST_DIR=\"$<TARGET_FILE_DIR:${test}>\""
    "IGN_TEST_LIBRARY_PATH=\"$<TARGET_FILE_DIR:${PROJECT_LIBRARY_TARGET_NAME}>\"")

endforeach()

if (TARGET UNIT_ign_TEST)
  set_tests_properties(
    UNIT_ign_TEST
    PROPERTIES
    ENVIRONMENT
      "IGN_CONFIG_PATH=${CMAKE_BINARY_DIR}/test/conf/$<CONFIG>"
  )
endif()

#===============================================================================
# Generate the ruby script for internal testing.
# Note that the major version of the library is included in the name.
# Ex: cmdtransport0.rb
set(cmd_script_generated_test "${CMAKE_BINARY_DIR}/test/lib/$<CONFIG>/ruby/ignition/cmd${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.rb")
set(cmd_script_configured_test "${CMAKE_CURRENT_BINARY_DIR}/test_cmd${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.rb.configured")

# Set the library_location variable to the full path of the library file within
# the build directory.
set(service_exe_location "$<TARGET_FILE:${service_executable}>")
set(topic_exe_location "$<TARGET_FILE:${topic_executable}>")

configure_file(
  "cmd${IGN_DESIGNATION}.rb.in"
  "${cmd_script_configured_test}"
  @ONLY)

file(GENERATE
  OUTPUT "${cmd_script_generated_test}"
  INPUT  "${cmd_script_configured_test}")


#===============================================================================
# Used for the installed version.
# Generate the ruby script that gets installed.
# Note that the major version of the library is included in the name.
# Ex: cmdtransport0.rb
set(cmd_script_generated "${CMAKE_CURRENT_BINARY_DIR}/cmd${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.rb")
set(cmd_script_configured "${cmd_script_generated}.configured")

# Set the library_location variable to the relative path to the library file
# within the install directory structure.
set(service_exe_location "../../../${CMAKE_INSTALL_LIBDIR}/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/$<TARGET_FILE_NAME:${service_executable}>")
set(topic_exe_location "../../../${CMAKE_INSTALL_LIBDIR}/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/$<TARGET_FILE_NAME:${topic_executable}>")

configure_file(
  "cmd${IGN_DESIGNATION}.rb.in"
  "${cmd_script_configured}"
  @ONLY)

file(GENERATE
  OUTPUT "${cmd_script_generated}"
  INPUT  "${cmd_script_configured}")

# Install the ruby command line library in an unversioned location.
install(FILES ${cmd_script_generated} DESTINATION lib/ruby/ignition)
