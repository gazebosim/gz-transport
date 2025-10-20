cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

project(gz-transport-examples-zenoh)

# Find Zenoh
find_package(zenoh_cpp_vendor QUIET)

# Find gz-msgs
find_package(gz-msgs QUIET REQUIRED)

if (EXISTS "${CMAKE_SOURCE_DIR}/zenoh_echo.cc")
  add_executable(zenoh_echo zenoh_echo.cc)
  target_link_libraries(zenoh_echo
    zenohcxx::zenohc
    gz-msgs::core
  )
endif()

if (MSVC)
  # Suppress Protobuf message generation warnings.
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4100 /wd4127 /wd4244 /wd4267 /wd4512")

  # Suppress the "decorated name length exceed" warning (inside the STL).
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503")

  add_definitions(-DBUILDING_STATIC_LIBS -DWIN32_LEAN_AND_MEAN)

  # Don't pull in the Windows min/max macros
  add_definitions(-DNOMINMAX)

endif()
