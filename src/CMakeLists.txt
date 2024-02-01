# Collect source files into the "sources" variable and unit test files into the
# "gtest_sources" variable.
gz_get_libsources_and_unittests(sources gtest_sources)

file(GLOB gz_msgs_proto_files
  ${CMAKE_CURRENT_SOURCE_DIR}/proto/gz/transport/private_msgs/*.proto
)

get_target_property(msgs_desc_file
  gz-msgs${GZ_MSGS_VER}::gz-msgs${GZ_MSGS_VER} GZ_MSGS_DESC_FILE)

gz_msgs_generate_messages_impl(
  MSGS_GEN_SCRIPT
    ${gz-msgs10_PROTO_GENERATOR_SCRIPT}
  FACTORY_GEN_SCRIPT
    ${gz-msgs10_FACTORY_GENERATOR_SCRIPT}
  GZ_PROTOC_PLUGIN
    ${gz-msgs10_PROTO_GENERATOR_PLUGIN}
  INPUT_PROTOS
    ${gz_msgs_proto_files}
  PROTO_PACKAGE
    "gz.transport.private_msgs"
  PROTO_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
  OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/gz_msgs_gen
  OUTPUT_SOURCES
    msgs_sources
  OUTPUT_HEADERS
    msgs_headers
  OUTPUT_DETAIL_HEADERS
    msgs_detail_headers
  OUTPUT_PYTHON
    msgs_python
  DEPENDENCY_DESCRIPTIONS ${msgs_desc_file}
)

# Create the library target.
gz_create_core_library(SOURCES ${sources} ${msgs_sources} CXX_STANDARD 17)

# Link the libraries that we always need.
target_link_libraries(${PROJECT_LIBRARY_TARGET_NAME}
  PUBLIC
    gz-utils${GZ_UTILS_VER}::gz-utils${GZ_UTILS_VER}
    gz-msgs${GZ_MSGS_VER}::gz-msgs${GZ_MSGS_VER}
    CPPZMQ::CPPZMQ
  PRIVATE
    ${ZeroMQ_TARGET}
)

target_include_directories(${PROJECT_LIBRARY_TARGET_NAME}
  SYSTEM PUBLIC 
    $<TARGET_PROPERTY:protobuf::libprotobuf,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:CPPZMQ::CPPZMQ,INTERFACE_INCLUDE_DIRECTORIES>)

# Windows system library provides UUID
if (NOT MSVC)
  target_link_libraries(${PROJECT_LIBRARY_TARGET_NAME}
    PUBLIC
      UUID::UUID
  )
endif()

# Build the unit tests.
gz_build_tests(TYPE UNIT SOURCES ${gtest_sources}
  TEST_LIST test_list
  LIB_DEPS ${EXTRA_TEST_LIB_DEPS})

foreach(test ${test_list})

  # Inform each test of its output directory so it knows where to call the
  # auxiliary files from. Using a generator expression here is useful for
  # multi-configuration generators, like Visual Studio.
  target_compile_definitions(${test} PRIVATE
    "DETAIL_GZ_TRANSPORT_TEST_DIR=\"$<TARGET_FILE_DIR:${test}>\""
    "GZ_TEST_LIBRARY_PATH=\"$<TARGET_FILE_DIR:${PROJECT_LIBRARY_TARGET_NAME}>\"")

endforeach()

if(MSVC)
  # On Windows, UNIT_Discovery_TEST uses some socket functions and therefore
  # needs to link to the Windows socket library. An easy, maintainable way to
  # do that is to tell it to link to the ZeroMQ target, and let it get the
  # socket library dependency transitively.
  if(TARGET UNIT_Discovery_TEST)
    target_link_libraries(UNIT_Discovery_TEST
      ${ZeroMQ_TARGET})
  endif()
endif()

# Command line support.
add_subdirectory(cmd)
