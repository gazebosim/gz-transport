set (cmake_find_files
  FindZeroMQ.cmake
)

set (CMAKE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules)
install(FILES ${cmake_find_files} DESTINATION ${CMAKE_INSTALL_DIR})
