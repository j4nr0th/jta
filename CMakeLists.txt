cmake_minimum_required(VERSION 3.25)
project(jta C)
include(CTest)

set(CMAKE_C_STANDARD 23)

list(APPEND UTILITY_SOURCE_FILES common/common.c)
list(APPEND UTILITY_HEADER_FILES common/common.h)

list(APPEND GFX_SOURCE_FILES
        gfx/drawing.c gfx/camera.c ui/jwin_handlers.c gfx/gfxerr.c gfx/bounding_box.c gfx/mesh.c
        gfx/vk_resources.c
        gfx/textures.c)
list(APPEND GFX_HEADER_FILES
        gfx/drawing.h gfx/camera.h ui/jwin_handlers.h gfx/gfxerr.h gfx/bounding_box.h gfx/mesh.h
        gfx/vk_resources.h
        gfx/textures.h)

list(APPEND JTA_SOLVER_SOURCE_FILES core/jtaerr.c core/jtapoints.c core/jtamaterials.c core/jtaprofiles.c
        core/jtaelements.c core/jtanaturalbcs.c core/jtanumericalbcs.c core/jtaproblem.c core/jtasolve.c)
list(APPEND JTA_SOLVER_HEADER_FILES core/jtaerr.h core/jtapoints.h core/jtamaterials.h core/jtaprofiles.h
        core/jtaelements.h core/jtanaturalbcs.h core/jtanumericalbcs.h core/jtaproblem.h core/jtasolve.h)



add_executable(jta main.c
        ${FORMATTED_IO_SOURCE_FILES} ${FORMATTED_IO_HEADER_FILES}
        ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES}
        ${GFX_SOURCE_FILES} ${GFX_HEADER_FILES}
        ${PARSING_SOURCE_FILES} ${PARSING_HEADER_FILES}
        ${UTILITY_SOURCE_FILES} ${UTILITY_HEADER_FILES}
        ${JTA_SOLVER_SOURCE_FILES} ${JTA_SOLVER_HEADER_FILES}
        config/config_loading.c config/config_loading.h gfx/color_map.c gfx/color_map.h
        ui/ui_tree.c
        ui/ui_tree.h
        gfx/frame_queue.c
        gfx/frame_queue.h
        ui/ui_sim_and_sol.c
        ui/ui_sim_and_sol.h
        jta_state.h
        ui/top_menu.c
        ui/top_menu.h
        core/jtaload.c
        core/jtaload.h
        core/jtaexport.c
        core/jtaexport.h)

add_compile_definitions(JALLOC_TRAP_COUNT=128)
add_compile_definitions(JALLOC_TRACKING=1)
add_compile_definitions(JDM_STACKTRACE=1)
add_compile_definitions(JVM_TRACK_ALLOCATIONS)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(jta PRIVATE -Wall -Wextra)
endif ()

include(cmake/Shaders.cmake)
shaders_target_setup()
add_shader_folder("${PROJECT_SOURCE_DIR}/shaders")

add_subdirectory(jio)
add_subdirectory(jmem)
add_subdirectory(jdm)
add_subdirectory(jmtx)
add_subdirectory(jwin)
add_subdirectory(jrui)
add_subdirectory(jvm)

configure_file(test_configuration.cfg "${CMAKE_BINARY_DIR}/test_configuration.cfg" COPYONLY)

target_include_directories(jta PRIVATE jio/include jdm/include jmem/source/include jmtx/source jrui/include jvm/include)
target_link_libraries(jta PRIVATE vulkan X11 m jmem jfmt jdm jio jmtx jwin jrui jvm shaders)


