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
install(TARGETS ${topic_executable} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/ignition/${GZ_DESIGNATION}${PROJECT_VERSION_MAJOR}/)

# Build service CLI executable
set(service_executable ign-transport-service)
add_executable(${service_executable} service_main.cc)
target_link_libraries(${service_executable}
  ign
  ignition-utils${IGN_UTILS_VER}::cli
  ${PROJECT_LIBRARY_TARGET_NAME}
)
install(TARGETS ${service_executable} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/ignition/${GZ_DESIGNATION}${PROJECT_VERSION_MAJOR}/)

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
    "IGN_TEST_LIBRARY_PATH=\"$<TARGET_FILE_DIR:${PROJECT_LIBRARY_TARGET_NAME}>\""
    "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")

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
ign_generate_cmd()

# Set the library_location variable to the relative path to the library file
# within the install directory structure.
set(service_exe_location "../../../${CMAKE_INSTALL_LIBEXECDIR}/ignition/${GZ_DESIGNATION}${PROJECT_VERSION_MAJOR}/$<TARGET_FILE_NAME:${service_executable}>")
set(topic_exe_location "../../../${CMAKE_INSTALL_LIBEXECDIR}/ignition/${GZ_DESIGNATION}${PROJECT_VERSION_MAJOR}/$<TARGET_FILE_NAME:${topic_executable}>")

#===============================================================================
# Bash completion

# Tack version onto and install the bash completion script
configure_file(
  "transport.bash_completion.sh"
    "${CMAKE_CURRENT_BINARY_DIR}/transport${PROJECT_VERSION_MAJOR}.bash_completion.sh" @ONLY)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/transport${PROJECT_VERSION_MAJOR}.bash_completion.sh
  DESTINATION
    ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/gz/gz${IGN_TOOLS_VER}.completion.d)
