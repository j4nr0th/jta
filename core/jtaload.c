//
// Created by jan on 22.9.2023.
//

#include <jdm.h>
#include "jtaload.h"

jta_result jta_load_points_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename)
{
    JDM_ENTER_FUNCTION;

    jta_point_list points;
    jio_memory_file* mem_file;

    const jio_result io_res = jio_memory_file_create(io_ctx, filename, &mem_file, 0, 0, 0);
    if (io_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load file \"%s\"", filename);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    const jta_result res = jta_load_points(io_ctx, mem_file, &points);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load point list, reason: %s", jta_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    jio_memory_file* old_file = problem->file_points;
    jta_point_list old_points = problem->point_list;

    problem->file_points = mem_file;
    problem->point_list = points;

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_POINTS)
    {
        jta_free_points(&old_points);
        jio_memory_file_destroy(old_file);
    }
    problem->load_state |= JTA_PROBLEM_LOAD_STATE_HAS_POINTS;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_materials_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename)
{
    JDM_ENTER_FUNCTION;

    jta_material_list materials;
    jio_memory_file* mem_file;

    const jio_result io_res = jio_memory_file_create(io_ctx, filename, &mem_file, 0, 0, 0);
    if (io_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load file \"%s\"", filename);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    const jta_result res = jta_load_materials(io_ctx, mem_file, &materials);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load material list, reason: %s", jta_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    jio_memory_file* old_file = problem->file_materials;
    jta_material_list old_materials = problem->material_list;

    problem->file_materials = mem_file;
    problem->material_list = materials;

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS)
    {
        jta_free_materials(&old_materials);
        jio_memory_file_destroy(old_file);
    }
    problem->load_state |= JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_profiles_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename)
{
    JDM_ENTER_FUNCTION;

    jta_profile_list profiles;
    jio_memory_file* mem_file;

    const jio_result io_res = jio_memory_file_create(io_ctx, filename, &mem_file, 0, 0, 0);
    if (io_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load file \"%s\"", filename);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    const jta_result res = jta_load_profiles(io_ctx, mem_file, &profiles);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load profile list, reason: %s", jta_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    jio_memory_file* old_file = problem->file_profiles;
    jta_profile_list old_profiles = problem->profile_list;

    problem->file_profiles = mem_file;
    problem->profile_list = profiles;

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_PROFILES)
    {
        jta_free_profiles(&old_profiles);
        jio_memory_file_destroy(old_file);
    }
    problem->load_state |= JTA_PROBLEM_LOAD_STATE_HAS_PROFILES;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_natbc_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename)
{
    JDM_ENTER_FUNCTION;

    if ((problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_POINTS) == 0)
    {
        JDM_ERROR("Can not load natural boundary conditions without loading points first");
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_PROBLEM;
    }

    jta_natural_boundary_condition_list natbcs;
    jio_memory_file* mem_file;

    const jio_result io_res = jio_memory_file_create(io_ctx, filename, &mem_file, 0, 0, 0);
    if (io_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load file \"%s\"", filename);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    const jta_result res = jta_load_natural_boundary_conditions(io_ctx, mem_file, &problem->point_list, &natbcs);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load natural boundary conditions list, reason: %s", jta_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    jio_memory_file* old_file = problem->file_nat;
    jta_natural_boundary_condition_list old_natbc = problem->natural_bcs;

    problem->file_nat = mem_file;
    problem->natural_bcs = natbcs;

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_NATBC)
    {
        jta_free_natural_boundary_conditions(&old_natbc);
        jio_memory_file_destroy(old_file);
    }
    problem->load_state |= JTA_PROBLEM_LOAD_STATE_HAS_NATBC;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_numbc_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename)
{

    JDM_ENTER_FUNCTION;

    if ((problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_POINTS) == 0)
    {
        JDM_ERROR("Can not load numerical boundary conditions without loading points first");
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_PROBLEM;
    }

    jta_numerical_boundary_condition_list numbcs;
    jio_memory_file* mem_file;

    const jio_result io_res = jio_memory_file_create(io_ctx, filename, &mem_file, 0, 0, 0);
    if (io_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load file \"%s\"", filename);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    const jta_result res = jta_load_numerical_boundary_conditions(io_ctx, mem_file, &problem->point_list, &numbcs);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load numural boundary conditions list, reason: %s", jta_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    jio_memory_file* old_file = problem->file_num;
    jta_numerical_boundary_condition_list old_numbc = problem->numerical_bcs;

    problem->file_num = mem_file;
    problem->numerical_bcs = numbcs;

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_NUMBC)
    {
        jta_free_numerical_boundary_conditions(&old_numbc);
        jio_memory_file_destroy(old_file);
    }
    problem->load_state |= JTA_PROBLEM_LOAD_STATE_HAS_NUMBC;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_elements_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename)
{
    JDM_ENTER_FUNCTION;

    if ((problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_POINTS) == 0
     || (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS) == 0
     || (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_PROFILES) == 0)
    {
        JDM_ERROR("Can not load elements without loading points, materials, and profiles first");
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_PROBLEM;
    }

    jta_element_list elements;
    jio_memory_file* mem_file;

    const jio_result io_res = jio_memory_file_create(io_ctx, filename, &mem_file, 0, 0, 0);
    if (io_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load file \"%s\"", filename);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    const jta_result res = jta_load_elements(io_ctx, mem_file, &problem->point_list, &problem->material_list, &problem->profile_list, &elements);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load element list, reason: %s", jta_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    jio_memory_file* old_file = problem->file_elements;
    jta_element_list old_elements = problem->element_list;

    problem->file_elements = mem_file;
    problem->element_list = elements;

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS)
    {
        jta_free_elements(&old_elements);
        jio_memory_file_destroy(old_file);
    }
    problem->load_state |= JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}
