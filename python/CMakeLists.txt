if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  # pybind11 logic for setting up a debug build when both a debug and release
  # python interpreter are present in the system seems to be pretty much broken.
  # This works around the issue.
  set(PYTHON_LIBRARIES "${PYTHON_DEBUG_LIBRARIES}")
endif()

if(USE_SYSTEM_PATHS_FOR_PYTHON_INSTALLATION)
  if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    execute_process(
      COMMAND "${PYTHON_EXECUTABLE}" -c "if True:
  from distutils import sysconfig as sc
  print(sc.get_python_lib(plat_specific=True))"
      OUTPUT_VARIABLE Python3_SITEARCH
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    # Get install variable from Python3 module
    # Python3_SITEARCH is available from 3.12 on, workaround if needed:
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
  set(GZ_PYTHON_INSTALL_PATH ${GZ_LIB_INSTALL_DIR}/python/gz)
endif()

# Set the build location and install location for a CPython extension
function(configure_build_install_location _library_name)
  # Install library for actual use
  install(TARGETS ${_library_name}
    DESTINATION "${GZ_PYTHON_INSTALL_PATH}/transport${PROJECT_VERSION_MAJOR}"
  )
endfunction()

message(STATUS "Building pybind11 interfaces")
# The bindings require a python wrapper in order to be able to provide an API
# similar to the one used in C++. We are creating the bindings name as the
# following in order to avoid having to modify the import on wrapper on each
# version of gz-transport.
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
        "${Python3_EXECUTABLE}" -m pytest "${CMAKE_SOURCE_DIR}/python/test/${test}.py" --junitxml "${CMAKE_BINARY_DIR}/test_results/${test}.xml")
    else()
      add_test(NAME ${test}.py COMMAND
        "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/python/test/${test}.py")
    endif()
    set(_env_vars)
    list(APPEND _env_vars "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin")
    list(APPEND _env_vars "PYTHONPATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/python/:${CMAKE_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
    list(APPEND _env_vars "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
    set_tests_properties(${test}.py PROPERTIES
      ENVIRONMENT "${_env_vars}")
  endforeach()
endif()
