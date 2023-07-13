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

mtx4 jta_element_transform_matrix(f32 dx, f32 dy, f32 dz);

void jta_element_stiffness_matrix(f32 dx, jmtx_matrix_dcm* out, f32 dy, f32 dz, f32 stiffness);

jta_result jta_make_global_matrices(
        const jta_point_list* point_list, const jta_element_list* element_list, const jta_profile_list* profiles,
        const jta_material_list* materials, f32 gravity, jmtx_matrix_crs* out_k, jmtx_matrix_crs* out_m, f32* out_f);

#endif //JTA_JTAMATRICES_H
