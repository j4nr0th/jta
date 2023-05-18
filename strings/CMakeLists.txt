add_executable(formatted_string_test formatted.c formatted.h tests/fmt_test.c ../mem/lin_jalloc.c ../mem/lin_jalloc.h ../profiling_timer.c ../profiling_timer.h)
target_link_libraries(formatted_string_test PRIVATE m)
add_test(NAME formatted_test COMMAND formatted_string_test)