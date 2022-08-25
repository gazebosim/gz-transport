
ign_get_libsources_and_unittests(sources gtest_sources)
list(APPEND sources cmd/ParamCommandAPI.cc)

ign_add_component(parameters SOURCES ${sources} GET_TARGET_NAME param_lib_target)

message(WARNING "param_lib_target ${param_lib_target}")

# Unit tests
ign_build_tests(
  TYPE "UNIT"
  SOURCES ${gtest_sources}
  LIB_DEPS ${param_lib_target} ${EXTRA_TEST_LIB_DEPS}
)

if(NOT WIN32)
  add_subdirectory(cmd)
endif()
