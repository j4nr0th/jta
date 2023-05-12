add_executable(aligned_allocator_test vk_allocator.c vk_allocator.h tests/aligned_test.c)
add_test(NAME aligned_test COMMAND aligned_allocator_test)