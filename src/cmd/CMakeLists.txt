include (${project_cmake_dir}/Utils.cmake)

# Generate a the ruby script.
# Note that the major version of the library is included in the name.
# Ex: cmdtransport0.rb

if (APPLE)
  set(IGN_LIBRARY_NAME lib${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}.dylib)
else()
  set(IGN_LIBRARY_NAME lib${PROJECT_NAME_LOWER}${PROJECT_MAJOR_VERSION}.so)
endif()

# Specify the library relative to the location of the .rb script.
# E.g.: cmdtransport3.rb is installed in:
#   /home/caguero/local/lib/ruby/ignition/cmdtransport3.rb
# The Ignition Transport library is installed in:
#   /home/caguero/local/lib/libignition-transport3.so
# The value of IGN_LIBRARY_NAME should be:
#   ../../libignition-transport3.so
set(IGN_LIBRARY_NAME ../../${IGN_LIBRARY_NAME})

configure_file(
  "cmdtransport.rb.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmdtransport${PROJECT_MAJOR_VERSION}.rb" @ONLY)

# Install the ruby command line library in an unversioned location.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmdtransport${PROJECT_MAJOR_VERSION}.rb DESTINATION lib/ruby/ignition)
