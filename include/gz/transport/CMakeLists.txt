# Exclude the detail directory from inclusion. The purpose is to prevent the
# detail/* header files from being included in transport.hh. A side effect is
# that the detail headers are not installed. The next install line solves this
# problem.
gz_install_all_headers(EXCLUDE_DIRS detail)
install(DIRECTORY detail DESTINATION ${GZ_INCLUDE_INSTALL_DIR_FULL}/gz/${GZ_DESIGNATION})
