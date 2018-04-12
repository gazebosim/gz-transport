add_executable(example_record record.cc)
target_link_libraries(example_record ${log_lib_target})

add_executable(example_playback playback.cc)
target_link_libraries(example_playback ${log_lib_target})

add_executable(example_custom_query custom_query.cc)
target_link_libraries(example_custom_query ${log_lib_target})
