//
// Created by jan on 13.7.2023.
//

#ifndef JTA_JTAMATRICES_H
#define JTA_JTAMATRICES_H
#include "jtaerr.h"
#include "jtaelements.h"
#include "jtapoints.h"
#include "../gfx/gfx_math.h"
#include <matrices/dense_col_major.h>
#include <matrices/sparse_row_compressed.h>

typedef struct jta_problem_setup_data_struct jta_problem_setup_data;
struct jta_problem_setup_data_struct
{
    const jta_point_list* point_list;       //  Point list parsed from input file
    const jta_element_list* element_list;   //  Element list parsed from input file
    const jta_profile_list* profile_list;   //  Profile list parsed from input file
    const jta_material_list* materials;     //  Material list parsed from input file
    vec4 gravity;                           //  Gravitational acceleration vector set in the input file
    jmtx_matrix_crs* stiffness_matrix;      //  Stiffness matrix K (size is 3 * length of point list in each dimension)
    f32* point_masses;                      //  Array of masses lumped at each point (length is same as point list)
    f32* forces;                            //  Forces vector f (size 3 * length of point list)
    f32* deformations;                      //  Deformation vector d (size 3 * length of point list)
};

mtx4 jta_element_transform_matrix(f32 dx, f32 dy, f32 dz);

void jta_element_stiffness_matrix(f32 dx, jmtx_matrix_dcm* out, f32 dy, f32 dz, f32 stiffness);

jta_result jta_make_global_matrices(
        const jta_point_list* point_list, const jta_element_list* element_list, const jta_profile_list* profiles,
        const jta_material_list* materials, f32 gravity, jmtx_matrix_crs* out_k, f32* out_m);

#endif //JTA_JTAMATRICES_H
