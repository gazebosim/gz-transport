# Detect if we are doing a standalone build of the bindings, using an external gz-transport
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.22.1)
  set(GZ_TRANSPORT_VER 14)
  project(gz-transport${GZ_TRANSPORT_VER}-python VERSION ${GZ_TRANSPORT_VER})
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  find_package(pybind11 REQUIRED)
  find_package(gz-transport${PROJECT_VERSION_MAJOR} REQUIRED)
  set(PROJECT_LIBRARY_TARGET_NAME "gz-transport${PROJECT_VERSION_MAJOR}::gz-transport${PROJECT_VERSION_MAJOR}")
  include(GNUInstallDirs)
  include(CTest)
endif()

if(USE_SYSTEM_PATHS_FOR_PYTHON_INSTALLATION)
  if(NOT Python3_SITEARCH)
    # Get install variable from Python3 module
    find_package(Python3 COMPONENTS Interpreter)
  endif()

  if(USE_DIST_PACKAGES_FOR_PYTHON)
    string(REPLACE "site-packages" "dist-packages" GZ_PYTHON_INSTALL_PATH ${Python3_SITEARCH})
  else()
    # custom cmake command is returning dist-packages
    string(REPLACE "dist-packages" "site-packages" GZ_PYTHON_INSTALL_PATH ${Python3_SITEARCH})
  endif()
else()
  # If not a system installation, respect local paths
  set(GZ_PYTHON_INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/python)
endif()

set(GZ_PYTHON_INSTALL_PATH "${GZ_PYTHON_INSTALL_PATH}/gz")

# Set the build location and install location for a CPython extension
function(configure_build_install_location _library_name)
  # Install library for actual use
  install(TARGETS ${_library_name}
    DESTINATION "${GZ_PYTHON_INSTALL_PATH}/transport${PROJECT_VERSION_MAJOR}"
  )
endfunction()

message(STATUS "Building pybind11 interfaces")
# We are creating the bindings name as the following because we 
# created a python wrapper that does some modifications in order
# to be able to provide an API similar to the one used in C++.
set(BINDINGS_MODULE_NAME "_transport")
# Split from main extension and converted to pybind11
pybind11_add_module(${BINDINGS_MODULE_NAME} MODULE
  src/transport/_gz_transport_pybind11.cc
)

target_link_libraries(${BINDINGS_MODULE_NAME} PRIVATE
  ${PROJECT_LIBRARY_TARGET_NAME}
)

target_compile_definitions(${BINDINGS_MODULE_NAME} PRIVATE
  BINDINGS_MODULE_NAME=${BINDINGS_MODULE_NAME})

configure_build_install_location(${BINDINGS_MODULE_NAME})

install(FILES
  src/__init__.py  
  DESTINATION "${GZ_PYTHON_INSTALL_PATH}/transport${PROJECT_VERSION_MAJOR}"
)

if (BUILD_TESTING AND NOT WIN32)
  set(python_tests
    pubSub_TEST
    requester_TEST
    options_TEST
  )
  execute_process(COMMAND "${Python3_EXECUTABLE}" -m pytest --version
    OUTPUT_VARIABLE PYTEST_output
    ERROR_VARIABLE  PYTEST_error
    RESULT_VARIABLE PYTEST_result)
  if(${PYTEST_result} EQUAL 0)
    set(pytest_FOUND TRUE)
  else()
    message(WARNING "Pytest package not available: ${PYTEST_error}")
  endif()

  foreach (test ${python_tests})
    if (pytest_FOUND)
      add_test(NAME ${test}.py COMMAND
        "${Python3_EXECUTABLE}" -m pytest "${CMAKE_CURRENT_SOURCE_DIR}/test/${test}.py" --junitxml "${CMAKE_BINARY_DIR}/test_results/${test}.xml")
    else()
      add_test(NAME ${test}.py COMMAND
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/${test}.py")
    endif()
    set(_env_vars)
    list(APPEND _env_vars "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin")
    list(APPEND _env_vars "PYTHONPATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/python/:${CMAKE_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
    list(APPEND _env_vars "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
    set_tests_properties(${test}.py PROPERTIES
      ENVIRONMENT "${_env_vars}")
  endforeach()
endif()
