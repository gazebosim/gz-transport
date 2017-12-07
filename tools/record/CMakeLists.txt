cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

project(ignition-transport-record)


find_package(ignition-transport3 REQUIRED)
find_package(ignition-msgs1 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(SQLite3 REQUIRED sqlite3>=3.6.19)

add_executable(record src/record.cpp)
target_link_libraries(record PRIVATE
  ${IGNITION-TRANSPORT_LIBRARIES}
  ${SQLite3_LIBRARIES}
)
target_include_directories(record PRIVATE
  ${IGNITION-TRANSPORT_INCLUDE_DIRS}
  ${SQLite3_INCLUD_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_options(record PRIVATE
  ${IGNITION-TRANSPORT_CXX_FLAGS}
  ${SQLite3_CFLAGS}
)

add_executable(talker src/talker.cpp)
target_link_libraries(talker PRIVATE ${IGNITION-TRANSPORT_LIBRARIES} ${ignition-msgs1_LIBRARIES})
target_include_directories(talker PRIVATE ${IGNITION-TRANSPORT_INCLUDE_DIRS})
target_compile_options(talker PRIVATE ${IGNITION-TRANSPORT_CXX_FLAGS})

set(SCHEMA_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/share)
configure_file(src/config.hh.in ${CMAKE_CURRENT_BINARY_DIR}/config.hh)
install(DIRECTORY schema DESTINATION ${SCHEMA_INSTALL_PATH})

install(TARGETS talker record
  ARCHIVE DESTINATION lib/static
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
