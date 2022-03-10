
ign_get_libsources_and_unittests(sources gtest_sources)
# list(APPEND sources cmd/LogCommandAPI.cc)

ign_add_component(parameters SOURCES ${sources} GET_TARGET_NAME parameters_lib_target)

# Unit tests
# ign_build_tests(
#   TYPE "UNIT"
#   SOURCES ${gtest_sources}
#   LIB_DEPS ${parameters_lib_target} ${EXTRA_TEST_LIB_DEPS}
# )

# target_include_directories(${parameters_lib_target}
#   PUBLIC
#     "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>")

# if(NOT WIN32)
#   add_subdirectory(cmd)
# endif()
