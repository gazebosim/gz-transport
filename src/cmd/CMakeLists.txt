# Set the library name.
if (APPLE)
  set(IGN_LIB_BASENAME lib${PROJECT_NAME_LOWER}.dylib)
else()
  set(IGN_LIB_BASENAME lib${PROJECT_NAME_LOWER}.so)
endif()

# Used only for internal testing.
set(IGN_LIBRARY_NAME ../../../../src/${IGN_LIB_BASENAME})

# Generate the ruby script for internal testing.
# Note that the major version of the library is included in the name.
# Ex: cmdtransport0.rb
configure_file(
  "cmdtransport.rb.in"
    "${CMAKE_BINARY_DIR}/test/lib/ruby/ignition/cmdtransport${PROJECT_VERSION_MAJOR}.rb" @ONLY)

# Used for the installed version.
#
# Specify the library path relative to the location of the .rb script.
# E.g.: cmdtransport3.rb is installed in:
#   /home/caguero/local/lib/ruby/ignition/cmdtransport3.rb
# The Ignition Transport library is installed in:
#   /home/caguero/local/lib/libignition-transport3.so
# The value of IGN_LIBRARY_NAME should be:
#   ../../libignition-transport3.so
set(IGN_LIBRARY_NAME ../../../${CMAKE_INSTALL_LIBDIR}/${IGN_LIB_BASENAME})

# Generate the ruby script that is installed.
# Note that the major version of the library is included in the name.
# Ex: cmdtransport0.rb
configure_file(
  "cmdtransport.rb.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmdtransport${PROJECT_VERSION_MAJOR}.rb" @ONLY)

# Install the ruby command line library in an unversioned location.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmdtransport${PROJECT_VERSION_MAJOR}.rb DESTINATION lib/ruby/ignition)
