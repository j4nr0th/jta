//
// Created by jan on 13.7.2023.
//

#ifndef JTA_JTAPROBLEM_H
#define JTA_JTAPROBLEM_H
#include "jtaerr.h"
#include "jtaelements.h"
#include "jtapoints.h"
#include "jtanaturalbcs.h"
#include "jtanumericalbcs.h"
#include "../gfx/gfx_math.h"
#include "../config/config_loading.h"
#include <matrices/dense_col_major.h>
#include <matrices/sparse_row_compressed.h>

typedef struct jta_problem_setup_struct jta_problem_setup;
struct jta_problem_setup_struct
{
    jio_memory_file *file_points, *file_materials, *file_profiles,
                    *file_elements, *file_nat, *file_num;          //  Files from which data was loaded (used to store labels)
    jta_point_list point_list;                           //  Point list parsed from input file
    jta_element_list element_list;                       //  Element list parsed from input file
    jta_profile_list profile_list;                       //  Profile list parsed from input file
    jta_material_list material_list;                         //  Material list parsed from input file
    jta_natural_boundary_condition_list natural_bcs;     //  Natural BC list parsed from input file
    jta_numerical_boundary_condition_list numerical_bcs; //  Numerical BC list parsed from input file
    vec4 gravity;                                               //  Gravitational acceleration vector set in the input file
};

jta_result jta_load_problem(const jio_context* io_ctx, const jta_config_problem* cfg, jta_problem_setup* problem);

void jta_free_problem(jta_problem_setup* problem);

mtx4 jta_element_transform_matrix(f32 dx, f32 dy, f32 dz);

void jta_element_stiffness_matrix(f32 dx, jmtx_matrix_dcm* out, f32 dy, f32 dz, f32 stiffness);

jta_result jta_make_global_matrices(
        const jta_point_list* point_list, const jta_element_list* element_list, const jta_profile_list* profiles,
        const jta_material_list* materials, jmtx_matrix_crs* out_k, f32* out_m);

jta_result jta_apply_natural_bcs(
        uint32_t n_pts, const jta_natural_boundary_condition_list* natural_bcs, vec4 gravity_acceleration,
        const f32* node_masses, f32* f_out);

jta_result jta_apply_numerical_bcs(const jta_point_list* point_list, const jta_numerical_boundary_condition_list* numerical_bcs, jmtx_matrix_crs* out_k, f32* f_out);

jta_result
jta_reduce_system(
        uint32_t n_pts, const jmtx_matrix_crs* k_g, const jta_numerical_boundary_condition_list* numerical_bcs,
        jmtx_matrix_crs** k_r, bool* dof_is_free);

#endif //JTA_JTAPROBLEM_H
