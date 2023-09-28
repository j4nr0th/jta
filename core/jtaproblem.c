//
// Created by jan on 13.7.2023.
//

#include <math.h>
#include "jtaproblem.h"
#include <matrices/sparse_row_compressed_internal.h>
#include <jdm.h>

static inline void jta_add_to_global_entries_3x3(const mtx4* mtx, uint32_t first_row, uint32_t first_col, jmtx_matrix_crs* out)
{
    int beef;
    (void)beef;
    if (mtx->s00 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 0, first_col + 0, mtx->s00);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
    if (mtx->s01 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 0, first_col + 1, mtx->s01);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
    if (mtx->s02 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 0, first_col + 2, mtx->s02);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }

    if (mtx->s10 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 1, first_col + 0, mtx->s10);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
    if (mtx->s11 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 1, first_col + 1, mtx->s11);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
    if (mtx->s12 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 1, first_col + 2, mtx->s12);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }

    if (mtx->s20 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 2, first_col + 0, mtx->s20);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
    if (mtx->s21 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 2, first_col + 1, mtx->s21);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
    if (mtx->s22 != 0)
    {
        jmtx_matrix_crs_add_to_entry(out, first_row + 2, first_col + 2, mtx->s22);
        assert(jmtx_matrix_crs_beef_check(out, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
    }
}

static inline void print_3x3(const mtx4* const mtx)
{
    printf("[[ %g %g %g],\n[%g %g %g],\n[%g %g %g]]\n", mtx->s00, mtx->s01, mtx->s02, mtx->s10, mtx->s11, mtx->s12, mtx->s20, mtx->s21, mtx->s22);
}

static inline void print_6x6(const mtx4* const a00, const mtx4* const a01, const mtx4* const a10, const mtx4* const a11)
{
    printf("[[% e % e % e % e % e % e],\n"
           "[% e % e % e % e % e % e],\n"
           "[% e % e % e % e % e % e],\n"
           "[% e % e % e % e % e % e],\n"
           "[% e % e % e % e % e % e],\n"
           "[% e % e % e % e % e % e]]\n",
           a00->s00, a00->s01, a00->s02, a01->s00, a01->s01, a01->s02,
           a00->s10, a00->s11, a00->s12, a01->s10, a01->s11, a01->s12,
           a00->s20, a00->s21, a00->s22, a01->s20, a01->s21, a01->s22,
           a10->s00, a10->s01, a10->s02, a11->s00, a11->s01, a11->s02,
           a10->s10, a10->s11, a10->s12, a11->s10, a11->s11, a11->s12,
           a10->s20, a10->s21, a10->s22, a11->s20, a11->s21, a11->s22
           );
}

static inline void print_global_matrix(const jmtx_matrix_crs* mtx)
{
    printf("Entries:");
    for (uint32_t i = 0, l = 0; i < mtx->n_entries; ++i)
    {
        while (l < mtx->base.rows && mtx->end_of_row_offsets[l] <= i)
        {
            l += 1;
        }
        printf(" (%u, %u, %g)", l, mtx->indices[i], mtx->values[i]);
    }
    printf("\n");
}

mtx4 jta_element_transform_matrix(
        f32 dx, f32 dy, f32 dz)
{
//    mtx4 result = {};
//    const f32 alpha = atan2f(dy, dx);
//    const f32 ca = cosf(alpha);
//    const f32 sa = sinf(alpha);
//    const f32 beta = - ((f32)M_PI_2 - atan2f(hypotf(dx, dy), dz));
//    const f32 cb = cosf(beta);
//    const f32 sb = sinf(beta);
//
//    result.col0.s0 = ca * cb;
//    result.col0.s1 = -sa;
//    result.col0.s2 = ca * sb;
//
//    result.col1.s0 = sa * cb;
//    result.col1.s1 = ca;
//    result.col1.s2 = sa * sb;
//
//    result.col2.s0 = -sb;
//    result.col2.s1 = 0;
//    result.col2.s2 = cb;

    mtx4 alt = {};
    const f32 mag_xy = hypotf(dx, dy);
    const f32 mag_all = hypotf(mag_xy, dz);
    const f32 rx = dx / mag_all;
    const f32 ry = dy / mag_all;
    const f32 rz = dz / mag_all;
    const f32 r_mag_xy = hypotf(rx, ry);

    alt.s00 = rx;
    alt.s01 = ry;
    alt.s02 = rz;

    alt.s12 = 0;
    if (r_mag_xy != 0.0f)
    {
        alt.s10 = - ry / r_mag_xy;
        alt.s11 = + rx / r_mag_xy;

        alt.s20 = - alt.s11 * rz;
        alt.s21 = + alt.s10 * rz;
        alt.s22 = r_mag_xy;
    }
    else
    {
        alt.s10 = 0;
        alt.s11 = 1;

        alt.s20 = -1;
        alt.s21 = 0;
        alt.s22 = 0;
    }


//    printf("\n\n");
//
//    print_3x3(&result);
//    print_3x3(&alt);
//    printf("\n\n");

    return alt;
}

jta_result jta_make_global_matrices(
        const jta_point_list* point_list, const jta_element_list* element_list, const jta_profile_list* profiles,
        const jta_material_list* materials, jmtx_matrix_crs* out_k, f32* out_m)
{
    assert(out_k->base.type == JMTX_TYPE_CRS);
    assert(out_k->base.rows == out_k->base.cols);
    assert(out_k->base.rows == 3 * point_list->count);
    assert(out_k->n_entries == 0);


    mtx4 mtx_a00 = {};
    mtx4 mtx_a01 = {};
    mtx4 mtx_a10 = {};
    mtx4 mtx_a11 = {};
    for (uint32_t i_element = 0; i_element < element_list->count; ++i_element)
    {
        const uint32_t i_pt0 = element_list->i_point0[i_element];
        const uint32_t i_pt1 = element_list->i_point1[i_element];
        const uint32_t i_mat = element_list->i_material[i_element];
        const uint32_t i_pro = element_list->i_profile[i_element];

        const f32 dx = point_list->p_x[i_pt1] -point_list->p_x[i_pt0];
        const f32 dy = point_list->p_y[i_pt1] -point_list->p_y[i_pt0];
        const f32 dz = point_list->p_z[i_pt1] -point_list->p_z[i_pt0];

        const f32 l = element_list->lengths[i_element];

        const mtx4 transform_mtx = jta_element_transform_matrix(dx, dy, dz);
        const mtx4 transposed_transform_matrix = mtx4_transpose(transform_mtx);

        const f32 stiffness_coefficient = profiles->area[i_pro] * materials->elastic_modulus[i_mat] / l;

        const f32 mass_of_element = profiles->area[i_pro] * l * materials->density[i_mat];

        mtx_a00.s00 = +stiffness_coefficient;
        mtx_a01.s00 = -stiffness_coefficient;
        mtx_a10.s00 = -stiffness_coefficient;
        mtx_a11.s00 = +stiffness_coefficient;

        mtx4 mtx_ta00 = mtx4_multiply3(mtx_a00, transform_mtx);
        mtx4 mtx_ta01 = mtx4_multiply3(mtx_a01, transform_mtx);
        mtx4 mtx_ta10 = mtx4_multiply3(mtx_a10, transform_mtx);
        mtx4 mtx_ta11 = mtx4_multiply3(mtx_a11, transform_mtx);

        mtx4 mtx_ta00t = mtx4_multiply3(transposed_transform_matrix, mtx_ta00);
        mtx4 mtx_ta01t = mtx4_multiply3(transposed_transform_matrix, mtx_ta01);
        mtx4 mtx_ta10t = mtx4_multiply3(transposed_transform_matrix, mtx_ta10);
        mtx4 mtx_ta11t = mtx4_multiply3(transposed_transform_matrix, mtx_ta11);
//        printf("Element %u stiffness matrix:\n", i_element);
//        print_6x6(&mtx_ta00t, &mtx_ta01t, &mtx_ta10t, &mtx_ta11t);
//        printf("\n\n");

        //  Now set the global matrix entries

        int beef = 0;
        (void)beef;
        jta_add_to_global_entries_3x3(&mtx_ta00t, 3 * i_pt0, 3 * i_pt0, out_k);
        assert(jmtx_matrix_crs_beef_check(out_k, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
        jta_add_to_global_entries_3x3(&mtx_ta01t, 3 * i_pt0, 3 * i_pt1, out_k);
        assert(jmtx_matrix_crs_beef_check(out_k, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
        jta_add_to_global_entries_3x3(&mtx_ta10t, 3 * i_pt1, 3 * i_pt0, out_k);
        assert(jmtx_matrix_crs_beef_check(out_k, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
        jta_add_to_global_entries_3x3(&mtx_ta11t, 3 * i_pt1, 3 * i_pt1, out_k);
        assert(jmtx_matrix_crs_beef_check(out_k, &beef) == JMTX_RESULT_SUCCESS);
        assert(beef == 0xBeef);
//        printf("After %u:\n", i_element);
//        print_global_matrix(out_k);
//        printf("\n\n");

        out_m[i_pt0] += mass_of_element / 2;
        out_m[i_pt1] += mass_of_element / 2;
    }
//    print_global_matrix(out_k);
//    exit(0);
    return JTA_RESULT_SUCCESS;
}

jta_result jta_apply_natural_bcs(
        uint32_t n_pts, const jta_natural_boundary_condition_list* natural_bcs, vec4 gravity_acceleration,
        const f32* restrict node_masses, f32* restrict f_out)
{
    //  First compute gravity
    u32 i = 0;
//    const __m128 g = _mm_load_ps(gravity_acceleration.data);
//    //  Overflow of values is fine, since they are being written over
//    for (i = 0; i < n_pts - 1; ++i)
//    {
//        __m128 m = _mm_set1_ps(node_masses[i]);
//        __m128 fg = _mm_mul_ps(g, m);
//        _mm_storeu_ps(f_out + 3 * i, fg);
//    }

    //  Finish up by doing it properly and not writing one extra component
    for (i *= 3; i < n_pts; ++i)
    {
        vec4 fg = vec4_mul_one(gravity_acceleration, node_masses[i]);
        f_out[3 * i + 0] = fg.x;
        f_out[3 * i + 1] = fg.y;
        f_out[3 * i + 2] = fg.z;
    }

    //  Add boundary conditions
    for (i = 0; i < natural_bcs->count; ++i)
    {
        const uint32_t i_pt = natural_bcs->i_point[i];
        f_out[3 * i_pt + 0] += natural_bcs->x[i];
        f_out[3 * i_pt + 1] += natural_bcs->y[i];
        f_out[3 * i_pt + 2] += natural_bcs->z[i];
    }

    return JTA_RESULT_SUCCESS;
}

jta_result jta_apply_numerical_bcs(
        const jta_point_list* point_list, const jta_numerical_boundary_condition_list* numerical_bcs, jmtx_matrix_crs* out_k, f32* f_out)
{
    JDM_ENTER_FUNCTION;

    for (uint32_t i = 0; i < numerical_bcs->count; ++i)
    {
        const uint32_t idx = numerical_bcs->count - 1 - i;
        const uint32_t i_pt = numerical_bcs->i_point[idx];
        const float val[1] = {1.0f};
        if (numerical_bcs->type[idx] & JTA_NUMERICAL_BC_TYPE_Z)
        {
            //  Set the matrix row
            uint32_t pos[1] = { 3 * i_pt + 2 };
            jmtx_result res = jmtx_matrix_crs_set_row(out_k, 3 * i_pt + 2, 1, pos, val);
            assert(res == JMTX_RESULT_SUCCESS);
            (void)res;
            f_out[3 * i_pt + 2] = point_list->p_z[i_pt] - numerical_bcs->z[idx];
        }

        if (numerical_bcs->type[idx] & JTA_NUMERICAL_BC_TYPE_Y)
        {
            //  Set the matrix row
            uint32_t pos[1] = { 3 * i_pt + 1 };
            jmtx_result res = jmtx_matrix_crs_set_row(out_k, 3 * i_pt + 1, 1, pos, val);
            assert(res == JMTX_RESULT_SUCCESS);
            (void)res;
            f_out[3 * i_pt + 1] = point_list->p_y[i_pt] - numerical_bcs->y[idx];
        }

        if (numerical_bcs->type[idx] & JTA_NUMERICAL_BC_TYPE_X)
        {
            //  Set the matrix row
            uint32_t pos[1] = { 3 * i_pt + 0 };
            jmtx_result res = jmtx_matrix_crs_set_row(out_k, 3 * i_pt + 0, 1, pos, val);
            assert(res == JMTX_RESULT_SUCCESS);
            (void)res;
            f_out[3 * i_pt + 0] = point_list->p_x[i_pt] - numerical_bcs->x[idx];
        }
    }

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result
jta_reduce_system(
        uint32_t n_pts, const jmtx_matrix_crs* k_g, const jta_numerical_boundary_condition_list* numerical_bcs,
        jmtx_matrix_crs** k_r, bool* dof_is_free)
{
    JDM_ENTER_FUNCTION;
    jmtx_matrix_crs* k;
    jmtx_result mtx_res;
    mtx_res = jmtx_matrix_crs_copy(k_g, &k, NULL);
    if (mtx_res != JMTX_RESULT_SUCCESS)
    {
        JDM_ERROR("Failed creating a copy of global stiffness matrix to create the reduced system, reason: %s",
                  jmtx_result_to_str(mtx_res));
        return JTA_RESULT_BAD_ALLOC;
    }

    memset(dof_is_free, 1, 3 * sizeof(*dof_is_free) * n_pts);

    for (uint32_t i = numerical_bcs->count; i != 0; --i)
    {
        const uint32_t i_pt = numerical_bcs->i_point[i - 1];
        const jta_numerical_boundary_condition_type type = numerical_bcs->type[i - 1];
        if (type & JTA_NUMERICAL_BC_TYPE_Z)
        {
            dof_is_free[3 * i_pt + 2] = 0;
        }
        if (type & JTA_NUMERICAL_BC_TYPE_Y)
        {
            dof_is_free[3 * i_pt + 1] = 0;
        }
        if (type & JTA_NUMERICAL_BC_TYPE_X)
        {
            dof_is_free[3 * i_pt + 0] = 0;
        }
    }

    for (uint32_t i = 3 * n_pts; i != 0; --i)
    {
        const uint32_t idx = i - 1;
        if (!dof_is_free[idx])
        {
            jmtx_matrix_crs_remove_row(k, idx);
        }
    }

    for (uint32_t i = 3 * n_pts; i != 0; --i)
    {
        const uint32_t idx = i - 1;
        if (!dof_is_free[idx])
        {
            jmtx_matrix_crs_remove_column(k, idx);
        }
    }

    *k_r = k;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_problem(const jio_context* io_ctx, const jta_config_problem* cfg, jta_problem_setup* problem)
{
    JDM_ENTER_FUNCTION;
    jio_result jio_res;
    jta_result res;

    jta_point_list point_list;
    jta_material_list material_list;
    jta_profile_list profile_list;
    jta_element_list element_list;
    jta_natural_boundary_condition_list natural_boundary_conditions;
    jta_numerical_boundary_condition_list numerical_boundary_conditions;
    memset(problem, 0, sizeof(*problem));
    jio_memory_file* p_point_file = NULL, * p_material_file = NULL, * p_profile_file = NULL, * p_element_file = NULL,
                   * p_natural_file = NULL, * p_numerical_file = NULL;

    jio_res = jio_memory_file_create(io_ctx, cfg->definition.points_file, &problem->file_points, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open point file \"%s\"", cfg->definition.points_file);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    p_point_file = problem->file_points;


    jio_res = jio_memory_file_create(io_ctx, cfg->definition.materials_file, &problem->file_materials, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open material file \"%s\"", cfg->definition.materials_file);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    p_material_file = problem->file_materials;
    
    jio_res = jio_memory_file_create(io_ctx, cfg->definition.profiles_file, &problem->file_profiles, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open profile file \"%s\"", cfg->definition.profiles_file);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    p_profile_file = problem->file_profiles;


    jio_res = jio_memory_file_create(io_ctx, cfg->definition.elements_file, &problem->file_elements, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open element file \"%s\"", cfg->definition.elements_file);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    p_element_file = problem->file_elements;


    jio_res = jio_memory_file_create(io_ctx, cfg->definition.natural_bcs_file, &problem->file_nat, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open element file \"%s\"", cfg->definition.natural_bcs_file);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    p_natural_file = problem->file_nat;

    jio_res = jio_memory_file_create(io_ctx, cfg->definition.numerical_bcs_file, &problem->file_num, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open element file \"%s\"", cfg->definition.numerical_bcs_file);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    p_numerical_file = problem->file_num;

    res = jta_load_points(io_ctx, p_point_file, &point_list);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load points");
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    if (point_list.count < 2)
    {
        JDM_ERROR("At least two points should be defined");
        res = JTA_RESULT_BAD_PROBLEM;
        goto failed;
    }

    res = jta_load_materials(io_ctx, p_material_file, &material_list);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load material_list");
        jta_free_points(&point_list);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    if (material_list.count < 1)
    {
        JDM_ERROR("At least one material should be defined");
        jta_free_points(&point_list);
        res = JTA_RESULT_BAD_PROBLEM;
        goto failed;
    }

    res = jta_load_profiles(io_ctx, p_profile_file, &profile_list);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load profiles");
        jta_free_points(&point_list);
        jta_free_materials(&material_list);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    if (profile_list.count < 1)
    {
        JDM_ERROR("At least one profile should be defined");
        jta_free_points(&point_list);
        jta_free_materials(&material_list);
        res = JTA_RESULT_BAD_PROBLEM;
        goto failed;
    }

    res = jta_load_elements(io_ctx, p_element_file, &point_list, &material_list, &profile_list, &element_list);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load element_list");
        jta_free_points(&point_list);
        jta_free_materials(&material_list);
        jta_free_profiles(&profile_list);
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }
    if (element_list.count < 1)
    {
        JDM_ERROR("At least one profile should be defined");
        jta_free_points(&point_list);
        jta_free_materials(&material_list);
        jta_free_profiles(&profile_list);
        res = JTA_RESULT_BAD_PROBLEM;
        goto failed;
    }

    res = jta_load_natural_boundary_conditions(io_ctx, p_natural_file, &point_list, &natural_boundary_conditions);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load natural boundary conditions");
        res = JTA_RESULT_BAD_IO;
        jta_free_points(&point_list);
        jta_free_materials(&material_list);
        jta_free_profiles(&profile_list);
        jta_free_elements(&element_list);
        goto failed;
    }

    res = jta_load_numerical_boundary_conditions(io_ctx, p_numerical_file, &point_list, &numerical_boundary_conditions);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load numerical boundary conditions");
        res = JTA_RESULT_BAD_IO;
        jta_free_points(&point_list);
        jta_free_materials(&material_list);
        jta_free_profiles(&profile_list);
        jta_free_elements(&element_list);
        jta_free_natural_boundary_conditions(&natural_boundary_conditions);
        goto failed;
    }

    problem->point_list = point_list;
    problem->material_list = material_list;
    problem->profile_list = profile_list;
    problem->element_list = element_list;
    problem->natural_bcs = natural_boundary_conditions;
    problem->numerical_bcs = numerical_boundary_conditions;

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
failed:
    if (p_numerical_file)
    {
        jio_memory_file_destroy(p_numerical_file);
    }
    if (p_natural_file)
    {
        jio_memory_file_destroy(p_natural_file);
    }
    if (p_element_file)
    {
        jio_memory_file_destroy(p_element_file);
    }
    if (p_profile_file)
    {
        jio_memory_file_destroy(p_profile_file);
    }
    if (p_material_file)
    {
        jio_memory_file_destroy(p_material_file);
    }
    if (p_point_file)
    {
        jio_memory_file_destroy(p_point_file);
    }
    problem->load_state = JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS|JTA_PROBLEM_LOAD_STATE_HAS_POINTS|JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS|
            JTA_PROBLEM_LOAD_STATE_HAS_PROFILES|JTA_PROBLEM_LOAD_STATE_HAS_NATBC|JTA_PROBLEM_LOAD_STATE_HAS_NUMBC;
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_free_problem(jta_problem_setup* problem)
{
    JDM_ENTER_FUNCTION;
    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_POINTS)
    {
        jta_free_points(&problem->point_list);
        jio_memory_file_destroy(problem->file_points);
        problem->load_state &= ~JTA_PROBLEM_LOAD_STATE_HAS_POINTS;
    }

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS)
    {
        jta_free_materials(&problem->material_list);
        jio_memory_file_destroy(problem->file_materials);
        problem->load_state &= ~JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS;
    }

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_PROFILES)
    {
        jta_free_profiles(&problem->profile_list);
        jio_memory_file_destroy(problem->file_profiles);
        problem->load_state &= ~JTA_PROBLEM_LOAD_STATE_HAS_PROFILES;
    }

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_NUMBC)
    {
        jio_memory_file_destroy(problem->file_num);
        jta_free_numerical_boundary_conditions(&problem->numerical_bcs);
        problem->load_state &= ~JTA_PROBLEM_LOAD_STATE_HAS_NUMBC;
    }

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_NATBC)
    {
        jio_memory_file_destroy(problem->file_nat);
        jta_free_natural_boundary_conditions(&problem->natural_bcs);
        problem->load_state &= ~JTA_PROBLEM_LOAD_STATE_HAS_NATBC;
    }

    if (problem->load_state & JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS)
    {
        jta_free_elements(&problem->element_list);
        jio_memory_file_destroy(problem->file_elements);
        problem->load_state &= ~JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS;
    }

    memset(problem, 0xCC, sizeof(*problem));
    problem->load_state = 0;
    JDM_LEAVE_FUNCTION;
}
