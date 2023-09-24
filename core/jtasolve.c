//
// Created by jan on 23.7.2023.
//
#include <inttypes.h>
#include <float.h>
#include <matrices/sparse_row_compressed.h>
#include <solvers/jacobi_point_iteration.h>
#include "jtasolve.h"
#include <jdm.h>

jta_result jta_solve_problem(const jta_config_problem* cfg, const jta_problem_setup* problem, jta_solution* solution)
{
    JDM_ENTER_FUNCTION;
    jmtx_matrix_crs* stiffness_matrix = NULL;
    const uint32_t point_count = problem->point_list.count;
    const uint32_t total_dofs = 3 * point_count;
    float* point_masses = NULL;
    float* forces = NULL;
    float* deformations = NULL;
    void* const base = lin_jallocator_save_state(G_LIN_JALLOCATOR);

    point_masses = ill_jalloc(G_JALLOCATOR, sizeof(*point_masses) * point_count);
    if (!point_masses)
    {
        JDM_ERROR("Could not allocate array for %zu point masses", (size_t) sizeof(*point_masses) * point_count);
        goto failed;
    }

    forces = ill_jalloc(G_JALLOCATOR, sizeof(*forces) * total_dofs);
    if (!forces)
    {
        JDM_ERROR("Could not allocate array for %zu element_forces", (size_t) sizeof(*forces) * total_dofs);
        goto failed;
    }

    deformations = ill_jalloc(G_JALLOCATOR, sizeof(*deformations) * total_dofs);
    if (!deformations)
    {
        JDM_ERROR("Could not allocate array for %zu point_displacements", (size_t) sizeof(*deformations) * total_dofs);
        goto failed;
    }

    const jmtx_allocator_callbacks allocator_callbacks =
            {
                    .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                    .realloc = (void* (*)(void*, void*, uint64_t)) ill_jrealloc,
                    .free = (void (*)(void*, void*)) ill_jfree,
                    .state = G_JALLOCATOR,
            };

    jta_result res;
    jmtx_result jmtx_res = jmtx_matrix_crs_new(
            &stiffness_matrix, total_dofs, total_dofs, 6 * 6 * total_dofs, &allocator_callbacks);
    if (jmtx_res != JMTX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create the system matrix of %zu by %zu, reason: %s", (size_t) total_dofs,
                  (size_t) total_dofs, jmtx_result_to_str(jmtx_res));
        res = JTA_RESULT_BAD_ALLOC;
        goto failed;
    }

    memset(point_masses, 0, sizeof(*point_masses) * point_count);
    res = jta_make_global_matrices(
            &problem->point_list, &problem->element_list, &problem->profile_list, &problem->material_list,
            stiffness_matrix, point_masses);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not build global system matrices, reason: %s", jta_result_to_str(res));
        goto failed;
    }

    memset(forces, 0, sizeof(*forces) * total_dofs);
    res = jta_apply_natural_bcs(point_count, &problem->natural_bcs, problem->gravity, point_masses, forces);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not apply natural BCs, reason: %s", jta_result_to_str(res));
        goto failed;
    }

    jmtx_matrix_crs* k_reduced;
    float* f_r = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*f_r) * total_dofs);
    if (!f_r)
    {
        JDM_ERROR("Could not allocate %zu bytes for reduced force vector", sizeof(*f_r) * total_dofs);
        res = JTA_RESULT_BAD_ALLOC;
        goto failed;
    }
    bool* dofs = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*dofs) * total_dofs);
    if (!dofs)
    {
        JDM_ERROR("Could not allocate %zu bytes for the list of free DOFs", sizeof(*dofs) * total_dofs);
        res = JTA_RESULT_BAD_ALLOC;
        lin_jfree(G_LIN_JALLOCATOR, f_r);
        goto failed;
    }
    float* u_r = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*u_r) * total_dofs);
    if (!u_r)
    {
        JDM_ERROR("Could not allocate %zu bytes for reduced force vector", sizeof(*u_r) * total_dofs);
        lin_jfree(G_LIN_JALLOCATOR, f_r);
        lin_jfree(G_LIN_JALLOCATOR, dofs);
        res = JTA_RESULT_BAD_ALLOC;
        goto failed;
    }


    res = jta_reduce_system(point_count, stiffness_matrix, &problem->numerical_bcs, &k_reduced, dofs);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not apply reduce the system of equations to solve, reason: %s", jta_result_to_str(res));
        lin_jfree(G_LIN_JALLOCATOR, f_r);
        lin_jfree(G_LIN_JALLOCATOR, u_r);
        lin_jfree(G_LIN_JALLOCATOR, dofs);
        goto failed;
    }

    uint32_t p_g, p_r;
    for (p_g = 0, p_r = 0; p_g < point_count * 3; ++p_g)
    {
        if (dofs[p_g])
        {
            f_r[p_r] = forces[p_g];
            p_r += 1;
        }
    }
    const uint32_t reduced_dofs = p_r;

    uint32_t iter_count;
    const uint32_t max_iters = cfg->sim_and_sol.max_iterations;    //  This could be config parameter
    float max_err = cfg->sim_and_sol.convergence_criterion;              //  This could be config parameter
    float final_error;
    jta_timer solver_timer;
    JDM_TRACE("Solving a %"PRIu32" by %"PRIu32" reduced system", reduced_dofs, reduced_dofs);
    jta_timer_set(&solver_timer);
    jmtx_res = jmtx_jacobi_relaxed_crs(
            k_reduced,
            f_r,
            u_r,
            cfg->sim_and_sol.relaxation_factor,
            max_err,
            max_iters,
            &iter_count,
            NULL,
            &final_error,
            &allocator_callbacks);
    f64 time_taken = jta_timer_get(&solver_timer);
    (void) jmtx_matrix_crs_destroy(k_reduced);

    JDM_INFO("Time taken for %"PRIu32" iterations of Jacobi was %g seconds, with a final error of %e", iter_count,
              time_taken, final_error);
    if (isnan(final_error))
    {
        JDM_ERROR("Failed solving the problem using Jacobi's method, error reached %g after %"PRIu32" iterations. This"
                  " likely means that the square of error reached %g, which gives %g as its sqrt.", final_error, iter_count, INFINITY, final_error);
        lin_jfree(G_LIN_JALLOCATOR, f_r);
        lin_jfree(G_LIN_JALLOCATOR, u_r);
        lin_jfree(G_LIN_JALLOCATOR, dofs);
        res = JTA_RESULT_BAD_PROBLEM;
        goto failed;
    }

    if (jmtx_res != JMTX_RESULT_SUCCESS && jmtx_res != JMTX_RESULT_NOT_CONVERGED)
    {
        JDM_ERROR("Failed solving the problem using Jacobi's method, reason: %s", jmtx_result_to_str(jmtx_res));
    }
    if (jmtx_res == JMTX_RESULT_NOT_CONVERGED)
    {
        JDM_WARN("Did not converge after %"PRIu32" iterations (error was %g)", iter_count, final_error);
    }

    //  Move to correct place
    for (p_g = 0, p_r = 0; p_g < point_count * 3; ++p_g)
    {
        if (dofs[p_g])
        {
            deformations[p_g] = u_r[p_r];
            p_r += 1;
        }
#ifndef NDEBUG
        else
        {
            deformations[p_g] = 0;
        }
#endif
    }
    lin_jfree(G_LIN_JALLOCATOR, u_r);
    u_r = NULL;
    lin_jfree(G_LIN_JALLOCATOR, dofs);
    dofs = NULL;

    //  Strongly apply numerical boundary conditions
    for (uint32_t i = 0; i < problem->numerical_bcs.count; ++i)
    {
        const jta_numerical_boundary_condition_type type = problem->numerical_bcs.type[i];
        const uint32_t i_pt = problem->numerical_bcs.i_point[i];
        if (type & JTA_NUMERICAL_BC_TYPE_X)
        {
#ifndef NDEBUG
            assert(deformations[3 * i_pt + 0] == 0);
#endif
            deformations[3 * i_pt + 0] = problem->point_list.p_x[i_pt] - problem->numerical_bcs.x[i];
        }
        if (type & JTA_NUMERICAL_BC_TYPE_Y)
        {

#ifndef NDEBUG
            assert(deformations[3 * i_pt + 1] == 0);
#endif
            deformations[3 * i_pt + 1] = problem->point_list.p_y[i_pt] - problem->numerical_bcs.y[i];
        }

        if (type & JTA_NUMERICAL_BC_TYPE_Z)
        {

#ifndef NDEBUG
            assert(deformations[3 * i_pt + 2] == 0);
#endif
            deformations[3 * i_pt + 2] = problem->point_list.p_z[i_pt] - problem->numerical_bcs.z[i];
        }
    }

    //  Compute reaction element_forces now that f_r is no longer in use
    jmtx_res = jmtx_matrix_crs_vector_multiply(stiffness_matrix, deformations, f_r);
    if (jmtx_res != JMTX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not compute reaction element_forces, reason: %s", jmtx_result_to_str(jmtx_res));
        goto failed;
    }

    //  Subtract natural bcs from f_r to find the actual reaction element_forces
    for (uint32_t i = 0; i < total_dofs; ++i)
    {
        forces[i] = f_r[i] - forces[i];
    }
    lin_jfree(G_LIN_JALLOCATOR, f_r);

    (void)jmtx_matrix_crs_destroy(stiffness_matrix);

    memset(solution, 0, sizeof(*solution));

    solution->point_count = problem->point_list.count;
    solution->point_reactions = forces;
    solution->point_displacements = deformations;
    solution->point_masses = point_masses;
    solution->final_residual_ratio = final_error;


    JDM_LEAVE_FUNCTION;
    return res;
failed:
    lin_jallocator_restore_current(G_LIN_JALLOCATOR, base);
    if (stiffness_matrix)
    {
        jmtx_matrix_crs_destroy(stiffness_matrix);
    }
    ill_jfree(G_JALLOCATOR, deformations);
    ill_jfree(G_JALLOCATOR, forces);
    ill_jfree(G_JALLOCATOR, point_masses);
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_solution_free(jta_solution* solution)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_JALLOCATOR, solution->point_masses);
    ill_jfree(G_JALLOCATOR, solution->point_reactions);
    ill_jfree(G_JALLOCATOR, solution->point_displacements);

    ill_jfree(G_JALLOCATOR, solution->element_masses);
    ill_jfree(G_JALLOCATOR, solution->element_forces);
    ill_jfree(G_JALLOCATOR, solution->element_stresses);

    JDM_LEAVE_FUNCTION;
}

jta_result jta_postprocess(const jta_problem_setup* problem, jta_solution* solution)
{
    JDM_ENTER_FUNCTION;
    const jta_point_list* const points = &problem->point_list;
    const jta_element_list* const elements = &problem->element_list;

    float* const stresses = ill_jalloc(G_JALLOCATOR, sizeof(*stresses) * elements->count);
    if (!stresses)
    {
        JDM_ERROR("Could not allocate memory for element stresses");
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_ALLOC;
    }
    float* const forces = ill_jalloc(G_JALLOCATOR, sizeof(*forces) * elements->count);
    if (!forces)
    {
        JDM_ERROR("Could not allocate memory for element forces");
        ill_jfree(G_JALLOCATOR, stresses);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_ALLOC;
    }
    float* const masses = ill_jalloc(G_JALLOCATOR, sizeof(*masses) * elements->count);
    if (!masses)
    {
        JDM_ERROR("Could not allocate memory for element masses");
        ill_jfree(G_JALLOCATOR, masses);
        ill_jfree(G_JALLOCATOR, stresses);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_ALLOC;
    }

    for (unsigned i_element = 0; i_element < elements->count; ++i_element)
    {
        const unsigned i_pt0 = elements->i_point0[i_element];
        const unsigned i_pt1 = elements->i_point1[i_element];
        const unsigned i_mat = elements->i_material[i_element];
        const unsigned i_pro = elements->i_profile[i_element];

        const vec4 original_direction = VEC4(
                points->p_x[i_pt1] - points->p_x[i_pt0],
                points->p_y[i_pt1] - points->p_y[i_pt0],
                points->p_z[i_pt1] - points->p_z[i_pt0]);
        const vec4 deformation = VEC4(
                solution->point_displacements[3lu * i_pt1 + 0] - solution->point_displacements[3lu * i_pt0 + 0],
                solution->point_displacements[3lu * i_pt1 + 1] - solution->point_displacements[3lu * i_pt0 + 1],
                solution->point_displacements[3lu * i_pt1 + 2] - solution->point_displacements[3lu * i_pt0 + 2]);
        const float original_len2 = vec4_dot(original_direction, original_direction);
        const float strain = vec4_dot(deformation, original_direction) / original_len2;
        const float stress = strain * problem->material_list.elastic_modulus[i_mat];
        stresses[i_element] = stress;
        const float area = problem->profile_list.area[i_pro];
        const float force = stress * area;
        forces[i_element] = force;
        const float mass = problem->material_list.density[i_mat] * area * sqrtf(original_len2);
        masses[i_element] = mass;
    }



    if (solution->element_count)
    {
        solution->element_count = 0;
        ill_jfree(G_JALLOCATOR, solution->element_masses);
        solution->element_masses = NULL;
        ill_jfree(G_JALLOCATOR, solution->element_forces);
        solution->element_forces = NULL;
        ill_jfree(G_JALLOCATOR, solution->element_stresses);
        solution->element_stresses = NULL;
    }
    solution->element_count = elements->count;
    solution->element_stresses = stresses;
    solution->element_forces = forces;
    solution->element_masses = masses;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}
