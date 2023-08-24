cmake_minimum_required(VERSION 3.25)
project(jta C)
include(CTest)

set(CMAKE_C_STANDARD 23)


list(APPEND JFW_SOURCE_FILES gfx/gfx_math.c jfw/jfw_common.c
        jfw/window.c jfw/jfw_error.c jfw/platform.c)
list(APPEND JFW_HEADER_FILES gfx/gfx_math.h jfw/jfw_common.h
        jfw/window.h jfw/jfw_error.h jfw/platform.h)

list(APPEND ALLOCATORS_SOURCE_FILES mem/vk_mem_allocator.c mem/aligned_jalloc.c)
list(APPEND ALLOCATORS_HEADER_FILES mem/vk_mem_allocator.h mem/aligned_jalloc.h)

list(APPEND UTILITY_SOURCE_FILES common/common.c)
list(APPEND UTILITY_HEADER_FILES common/common.h)

list(APPEND GFX_SOURCE_FILES
        #gfx/vk_state.c
        gfx/drawing_3d.c gfx/camera.c jwin_handlers.c gfx/gfxerr.c gfx/bounding_box.c gfx/mesh.c)
list(APPEND GFX_HEADER_FILES
        #gfx/vk_state.h
        gfx/drawing_3d.h gfx/camera.h jwin_handlers.h gfx/gfxerr.h gfx/bounding_box.h gfx/mesh.h)

list(APPEND JTA_SOLVER_SOURCE_FILES core/jtaerr.c core/jtapoints.c core/jtamaterials.c core/jtaprofiles.c
        core/jtaelements.c core/jtanaturalbcs.c core/jtanumericalbcs.c core/jtaproblem.c core/jtasolve.c)
list(APPEND JTA_SOLVER_HEADER_FILES core/jtaerr.h core/jtapoints.h core/jtamaterials.h core/jtaprofiles.h
        core/jtaelements.h core/jtanaturalbcs.h core/jtanumericalbcs.h core/jtaproblem.h core/jtasolve.h)

include(cmake/Shaders.cmake)
shaders_target_setup()
add_shader_folder("${PROJECT_SOURCE_DIR}/shaders")

add_subdirectory(jio)
add_subdirectory(jmem)
add_subdirectory(jdm)
add_subdirectory(jmtx)
add_subdirectory(jwin)
add_subdirectory(jrui)


add_custom_command(
        OUTPUT shaders/frg_shader3d.spv shaders/vtx_shader3d.spv shaders/frg_shader_ui.spv shaders/vtx_shader_ui.spv
        COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile_shaders.sh shaders
        DEPENDS shaders/compile_shaders.sh shaders/frg_shader3d.frag shaders/vtx_shader3d.vert shaders/frg_shader_ui.frag shaders/vtx_shader_ui.vert)

list(APPEND JTA_SHADERS shaders/frg_shader3d.spv shaders/vtx_shader3d.spv shaders/frg_shader_ui.spv shaders/vtx_shader_ui.spv)
add_executable(jta main.c
        ${ALLOCATORS_SOURCE_FILES} ${ALLOCATORS_HEADER_FILES}
        ${FORMATTED_IO_SOURCE_FILES} ${FORMATTED_IO_HEADER_FILES}
        ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES}
        ${GFX_SOURCE_FILES} ${GFX_HEADER_FILES}
        ${PARSING_SOURCE_FILES} ${PARSING_HEADER_FILES}
        ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES}
        ${JTA_SOLVER_SOURCE_FILES} ${JTA_SOLVER_HEADER_FILES}
        ${JTA_SHADERS}
        config/config_loading.c config/config_loading.h gfx/color_map.c gfx/color_map.h config/config_execution.c config/config_execution.h
        gfx/vk_resources.c
        gfx/vk_resources.h
        gfx/textures.c
        gfx/textures.h
        ui_tree.c
        ui_tree.h)

add_compile_definitions(JALLOC_TRAP_COUNT=128)
add_compile_definitions(JALLOC_TRACKING=1)
add_compile_definitions(JDM_STACKTRACE=1)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(jta PRIVATE -Wall -Wextra)
endif ()


configure_file(test_configuration.cfg "${CMAKE_BINARY_DIR}/test_configuration.cfg" COPYONLY)

target_include_directories(jta PRIVATE jio/include jdm/include jmem/source/include jmtx/source jrui/include)
target_link_libraries(jta PRIVATE vulkan X11 m jmem jfmt jdm jio jmtx jwin jrui shaders)

enable_testing()
#add_executable(geo_test gfx/gfx_math.h gfx/gfx_math.c gfx/camera.c gfx/camera.h gfx/drawing_3d.h gfx/drawing_3d.c ${JFW_SOURCE_FILES} ${JFW_HEADER_FILES} gfx/rotation_camera_test.c)
#target_link_libraries(geo_test PRIVATE m X11 vulkan jdm)
add_subdirectory(mem)

