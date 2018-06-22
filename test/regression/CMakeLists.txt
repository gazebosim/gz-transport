set(TEST_TYPE "REGRESSION")

ign_get_sources(tests)

ign_build_tests(TYPE REGRESSION SOURCES ${tests})

if(TARGET REGRESSION_deprecated)
  # Turn off deprecation warnings specifically for the test of deprecated
  # functions.
  if(UNIX)
    target_compile_options(REGRESSION_deprecated PRIVATE -Wno-deprecated-declarations)
  elseif(MSVC)
    target_compile_options(REGRESSION_deprecated PRIVATE /wd4996)
  endif()
endif()
