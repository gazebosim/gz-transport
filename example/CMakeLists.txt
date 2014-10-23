cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Find the Ignition_Transport library
find_package(ignition-transport QUIET REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNITION-TRANSPORT_CXX_FLAGS}")

include_directories(${IGNITION-TRANSPORT_INCLUDE_DIRS})
link_directories(${IGNITION-TRANSPORT_LIBRARY_DIRS})

#################################################
# Find cppzeromq header (shipped together with zeromq in debian/ubuntu but
# different upstream projects and tarballs)
# 
# Provide the PATH using CPPZMQ_HEADER_PATH
#
find_path(cppzmq_INCLUDE_DIRS 
          zmq.hpp 
	  PATHS 
	   ${zmq_INCLUDE_DIRS}
	   ${CPPZMQ_HEADER_PATH})

if (NOT cppzmq_INCLUDE_DIRS)
  message(STATUS "cppzmq header file was not found")
  BUILD_ERROR("cppzmq header file was not found")
else()
  message(STATUS "cppzmq file - found")
  include_directories(${cppzmq_INCLUDE_DIRS})
endif()

#################################################
# Find ZeroMQ.
include (${PROJECT_SOURCE_DIR}/cmake/FindZeroMQ.cmake)

if (NOT ZeroMQ_FOUND)
  BUILD_ERROR ("zmq not found, Please install zmq")
else ()
  include_directories(${ZeroMQ_INCLUDE_DIRS})
  link_directories(${ZeroMQ_LIBRARY_DIRS})
endif ()

add_subdirectory(msg)

set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER} PROPERTIES
                            GENERATED TRUE)
include_directories(${CMAKE_BINARY_DIR})

if(WIN32)
  set(EXTRA_LIBRARIES ${ZeroMQ_LIBRARIES} wsock32 ws2_32 Iphlpapi)
endif()

add_executable(publisher publisher.cc ${PROTO_SRC})
target_link_libraries(publisher ${IGNITION-TRANSPORT_LIBRARIES} ${EXTRA_LIBRARIES})
add_dependencies(publisher protobuf_compilation)

add_executable(subscriber subscriber.cc ${PROTO_SRC})
target_link_libraries(subscriber ${IGNITION-TRANSPORT_LIBRARIES} ${EXTRA_LIBRARIES})
add_dependencies(subscriber protobuf_compilation)

add_executable(responser responser.cc ${PROTO_SRC})
target_link_libraries(responser ${IGNITION-TRANSPORT_LIBRARIES} ${EXTRA_LIBRARIES})
add_dependencies(responser protobuf_compilation)

add_executable(requester requester.cc ${PROTO_SRC})
target_link_libraries(requester ${IGNITION-TRANSPORT_LIBRARIES} ${EXTRA_LIBRARIES})
add_dependencies(requester protobuf_compilation)

add_executable(requester_async requester_async.cc ${PROTO_SRC})
target_link_libraries(requester_async ${IGNITION-TRANSPORT_LIBRARIES} ${EXTRA_LIBRARIES})
add_dependencies(requester_async protobuf_compilation)
