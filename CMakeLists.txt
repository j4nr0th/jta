cmake_minimum_required(VERSION 3.25)
project(jtb C)
include(CTest)

set(CMAKE_C_STANDARD 23)

#add_compile_options("-march=native")

list(APPEND JFW_SOURCE_FILES gfx/gfx_math.c jfw/jfw_common.c jfw/unicode_block_lookup_table.c jfw/widget-base.c
        jfw/window.c jfw/error_system/error_codes.c jfw/vk/platform.c)
list(APPEND JFW_HEADER_FILES gfx/gfx_math.h jfw/jfw_common.h jfw/unicode_block_lookup_table.h jfw/widget-base.h
        jfw/window.h jfw/error_system/error_codes.h jfw/vk/platform.h)

list(APPEND ALLOCATORS_SOURCE_FILES mem/vk_mem_allocator.c mem/aligned_jalloc.c)
list(APPEND ALLOCATORS_HEADER_FILES mem/vk_mem_allocator.h mem/aligned_jalloc.h)

list(APPEND UTILITY_SOURCE_FILES common/common.c)
list(APPEND UTILITY_HEADER_FILES common/common.h)

list(APPEND GFX_SOURCE_FILES gfx/vk_state.c gfx/drawing_3d.c gfx/camera.c ui.c)
list(APPEND GFX_HEADER_FILES gfx/vk_state.h gfx/drawing_3d.h gfx/camera.h ui.h)

list(APPEND JIO_SOURCE_FILES io/iobase.c io/ioerr.c io/iocsv.c)
list(APPEND JIO_HEADER_FILES io/iobase.h io/ioerr.h io/iocsv.h)

add_executable(jtb main.c
        ${JFW_SOURCE_FILES} ${JFW_HEADER_FILES}
        ${ALLOCATORS_SOURCE_FILES} ${ALLOCATORS_HEADER_FILES}
        ${FORMATTED_IO_SOURCE_FILES} ${FORMATTED_IO_HEADER_FILES}
        ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES}
        ${GFX_SOURCE_FILES} ${GFX_HEADER_FILES}
        ${PARSING_SOURCE_FILES} ${PARSING_HEADER_FILES}
        ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES}
        ${JIO_SOURCE_FILES} ${JIO_HEADER_FILES}
        mem/vk_mem_allocator.c mem/vk_mem_allocator.h gfx/gfxerr.c gfx/gfxerr.h gfx/gfxerr.c)
target_link_libraries(jtb PRIVATE vulkan X11 m jmem jfmt jdm)

add_executable(jfw_test ${JFW_SOURCE_FILES} ${JFW_HEADER_FILES} jfw/main.c)
target_link_libraries(jfw_test PRIVATE vulkan X11 m jmem jfmt jdm)
enable_testing()
add_executable(geo_test gfx/gfx_math.h gfx/gfx_math.c geo_test.c gfx/camera.c gfx/camera.h gfx/drawing_3d.h gfx/drawing_3d.c ${JFW_SOURCE_FILES} ${JFW_HEADER_FILES} gfx/rotation_camera_test.c)
target_link_libraries(geo_test PRIVATE m X11 vulkan jdm)
add_subdirectory(mem)
add_subdirectory(io)
