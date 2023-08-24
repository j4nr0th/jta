function(shaders_target_setup)
    if (NOT EXISTS ${CMAKE_BINARY_DIR}/shaders)
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    endif ()

    if (NOT EXISTS ${CMAKE_BINARY_DIR}/shaders/shaders_empty.c)
        file(WRITE ${CMAKE_BINARY_DIR}/shaders/shaders_empty.c "")
    endif ()

    add_library(shaders ${CMAKE_BINARY_DIR}/shaders/shaders_empty.c)
    target_include_directories(shaders PUBLIC ${CMAKE_BINARY_DIR}/shaders)
endfunction()

function(add_shader_folder folder)
    file(GLOB_RECURSE shaders_SRC CONFIGURE_DEPENDS "${folder}/*.frag" "${folder}/*.vert")
    set(shaders_base_path ${CMAKE_BINARY_DIR}/shaders)

    foreach(source ${shaders_SRC})

        cmake_path(RELATIVE_PATH source BASE_DIRECTORY ${folder} OUTPUT_VARIABLE source_relative)
        cmake_path(APPEND shaders_base_path ${source_relative} OUTPUT_VARIABLE shader_moved)

        compile_shader(${source} ${shader_moved})

    endforeach()
endfunction()

function(compile_shader source shader_moved)
    get_filename_component(base_filename ${source} NAME)
    cmake_path(GET shader_moved PARENT_PATH shader_workdir)
    string(MAKE_C_IDENTIFIER ${base_filename} c_name)

    add_custom_command(
            OUTPUT ${shader_moved}.c ${shader_moved}.h
            DEPENDS ${source}
            WORKING_DIRECTORY ${shader_workdir}
            COMMAND
            ${PROJECT_SOURCE_DIR}/cmake/compile_shader.sh
            ${c_name}
            vulkan1.0
            ${base_filename}
            ${source}
    )

    if (NOT EXISTS ${CMAKE_BINARY_DIR}/shaders)
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    endif ()

    target_sources(shaders PUBLIC ${shader_moved}.c)
endfunction()
