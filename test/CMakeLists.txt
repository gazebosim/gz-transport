add_subdirectory(gtest_vendor)
add_subdirectory(integration)
add_subdirectory(performance)
add_subdirectory(regression)

configure_file (test_config.hh.in ${PROJECT_BINARY_DIR}/include/test_config.hh)
add_library(test_config INTERFACE)
target_include_directories(test_config INTERFACE
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/test
)
