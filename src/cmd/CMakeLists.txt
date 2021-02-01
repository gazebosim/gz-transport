# Collect source files into the "sources" variable and unit test files into the
# "gtest_sources" variable.
ign_get_libsources_and_unittests(sources gtest_sources)

# Make a small static lib of command line functions
add_library(ign STATIC ign.cc)
target_link_libraries(ign
  ${PROJECT_LIBRARY_TARGET_NAME}
)

# Build topic CLI executable
set(executable ign-transport-topic)
add_executable(${executable} topic_main.cc)
target_link_libraries(${executable}
  ign
  ignition-utils${IGN_UTILS_VER}::cli
  ${PROJECT_LIBRARY_TARGET_NAME}
)
install(TARGETS ${executable} DESTINATION lib/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/)

# Build service CLI executable
set(executable ign-transport-service)
add_executable(${executable} service_main.cc)
target_link_libraries(${executable}
  ign
  ignition-utils${IGN_UTILS_VER}::cli
  ${PROJECT_LIBRARY_TARGET_NAME}
)
install(TARGETS ${executable} DESTINATION lib/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/)

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

#===============================================================================
# Generate the ruby script for internal testing.
# Note that the major version of the library is included in the name.
# Ex: cmdtransport0.rb
set(cmd_script_generated_test "${CMAKE_BINARY_DIR}/test/lib/ruby/ignition/cmd${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}.rb")
set(cmd_script_configured_test "${cmd_script_generated_test}.configured")

# Set the library_location variable to the full path of the library file within
# the build directory.
set(exe_location "$<TARGET_FILE:${executable}>")

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
set(exe_location "../../../${CMAKE_INSTALL_LIBDIR}/ignition/${IGN_DESIGNATION}${PROJECT_VERSION_MAJOR}/$<TARGET_FILE_NAME:${executable}>")

configure_file(
  "cmd${IGN_DESIGNATION}.rb.in"
  "${cmd_script_configured}"
  @ONLY)

file(GENERATE
  OUTPUT "${cmd_script_generated}"
  INPUT  "${cmd_script_configured}")

# Install the ruby command line library in an unversioned location.
install(FILES ${cmd_script_generated} DESTINATION lib/ruby/ignition)
