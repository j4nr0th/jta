cmake_minimum_required(VERSION 3.25)
project(jtb C)
include(CTest)

set(CMAKE_C_STANDARD 23)

list(APPEND JFW_SOURCE_FILES jfw/gfx_math.c jfw/jfw_common.c jfw/unicode_block_lookup_table.c jfw/widget-base.c
        jfw/window.c jfw/error_system/error_codes.c jfw/error_system/error_stack.c jfw/vk/platform.c)
list(APPEND JFW_HEADER_FILES jfw/gfx_math.h jfw/jfw_common.h jfw/unicode_block_lookup_table.h jfw/widget-base.h
        jfw/window.h jfw/error_system/error_codes.h jfw/error_system/error_stack.h jfw/vk/platform.h)

list(APPEND ALLOCATORS_SOURCE_FILES mem/jalloc.c mem/lin_alloc.c mem/aligned_jalloc.c)
list(APPEND ALLOCATORS_HEADER_FILES mem/jalloc.h mem/lin_alloc.h mem/aligned_jalloc.h)

list(APPEND FORMATTED_IO_SOURCE_FILES strings/formatted.c)
list(APPEND FORMATTED_IO_HEADER_FILES strings/formatted.h)

list(APPEND UTILITY_SOURCE_FILES profiling_timer.c)
list(APPEND UTILITY_HEADER_FILES profiling_timer.h)

add_executable(jtb main.c ${JFW_SOURCE_FILES} ${JFW_HEADER_FILES} ${ALLOCATORS_SOURCE_FILES} ${ALLOCATORS_HEADER_FILES}   ${FORMATTED_IO_SOURCE_FILES} ${FORMATTED_IO_HEADER_FILES} ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES})
target_link_libraries(jtb PRIVATE vulkan X11)

enable_testing()
add_subdirectory(mem)
add_subdirectory(strings)