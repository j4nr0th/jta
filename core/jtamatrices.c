//
// Created by jan on 13.7.2023.
//

#include <math.h>
#include "jtamatrices.h"

mtx4 jta_element_transform_matrix(
        f32 dx, f32 dy, f32 dz)
{
    mtx4 result = {};
    const f32 alpha = atan2f(dy, dx);
    const f32 ca = cosf(alpha);
    const f32 sa = sinf(alpha);
    const f32 beta = - (f32)M_PI_2 - atan2f(hypotf(dx, dy), dz);
    const f32 cb = cosf(beta);
    const f32 sb = sinf(beta);

    result.col0.s0 = ca * cb;
    result.col0.s1 = -sa;
    result.col0.s2 = ca * sb;

    result.col1.s0 = sa * cb;
    result.col1.s1 = ca;
    result.col1.s2 = sa * sb;

    result.col2.s0 = -sb;
    result.col2.s1 = 0;
    result.col2.s2 = cb;

    return result;
}

static inline void jta_add_to_global_entries_3x3(const mtx4* mtx, uint32_t first_row, uint32_t first_col, jmtx_matrix_crs* out)
{
    jmtx_matrix_crs_add_to_element(out, first_row + 0, first_col + 0, mtx->s00);
    jmtx_matrix_crs_add_to_element(out, first_row + 0, first_col + 1, mtx->s01);
    jmtx_matrix_crs_add_to_element(out, first_row + 0, first_col + 2, mtx->s02);

    jmtx_matrix_crs_add_to_element(out, first_row + 1, first_col + 0, mtx->s10);
    jmtx_matrix_crs_add_to_element(out, first_row + 1, first_col + 1, mtx->s11);
    jmtx_matrix_crs_add_to_element(out, first_row + 1, first_col + 2, mtx->s12);

    jmtx_matrix_crs_add_to_element(out, first_row + 2, first_col + 0, mtx->s20);
    jmtx_matrix_crs_add_to_element(out, first_row + 2, first_col + 1, mtx->s21);
    jmtx_matrix_crs_add_to_element(out, first_row + 2, first_col + 2, mtx->s22);
}

jta_result jta_make_global_matrices(
        const jta_point_list* point_list, const jta_element_list* element_list, const jta_profile_list* profiles,
        const jta_material_list* materials, f32 gravity, jmtx_matrix_crs* out_k, f32* out_m)
{
    assert(out_k->base.type == JMTX_TYPE_CRS);
    assert(out_k->base.rows == out_k->base.cols);
    assert(out_k->base.rows == 3 * point_list->count);
    assert(out_k->n_elements == 0);


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

        mtx4 mtx_ta00 = mtx4_multiply3(transform_mtx, mtx_a00);
        mtx4 mtx_ta01 = mtx4_multiply3(transform_mtx, mtx_a01);
        mtx4 mtx_ta10 = mtx4_multiply3(transform_mtx, mtx_a10);
        mtx4 mtx_ta11 = mtx4_multiply3(transform_mtx, mtx_a11);

        mtx4 mtx_ta00t = mtx4_multiply3(mtx_ta00, transposed_transform_matrix);
        mtx4 mtx_ta01t = mtx4_multiply3(mtx_ta01, transposed_transform_matrix);
        mtx4 mtx_ta10t = mtx4_multiply3(mtx_ta10, transposed_transform_matrix);
        mtx4 mtx_ta11t = mtx4_multiply3(mtx_ta11, transposed_transform_matrix);

        //  Now set the global matrix entries

        jta_add_to_global_entries_3x3(&mtx_ta00t, i_pt0, i_pt0, out_k);
        jta_add_to_global_entries_3x3(&mtx_ta01t, i_pt0, i_pt1, out_k);
        jta_add_to_global_entries_3x3(&mtx_ta10t, i_pt1, i_pt0, out_k);
        jta_add_to_global_entries_3x3(&mtx_ta11t, i_pt1, i_pt1, out_k);

        out_m[i_pt0] += gravity * mass_of_element / 2;
        out_m[i_pt1] += gravity * mass_of_element / 2;
    }

    return JTA_RESULT_SUCCESS;
}
