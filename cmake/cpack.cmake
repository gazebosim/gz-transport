################################################################################
#Find available package generators

list (APPEND CPACK_SOURCE_GENERATOR "TBZ2")
list (APPEND CPACK_SOURCE_GENERATOR "ZIP")
list (APPEND CPACK_SOURCE_IGNORE_FILES "TODO;/.hg/;.swp$;/build/;.hgtags")

include (InstallRequiredSystemLibraries)
set (PROJECT_CPACK_CFG_FILE "${PROJECT_BINARY_DIR}/cpack_options.cmake")
