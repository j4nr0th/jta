//
// Created by jan on 5.7.2023.
//

#ifndef JTA_JTAMATERIALS_H
#define JTA_JTAMATERIALS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtaerr.h"



typedef struct jta_material_struct jta_material;
struct jta_material_struct
{
    f64 density;
    f64 elastic_modulus;
    f64 tensile_strength;
    f64 compressive_strength;
    jio_string_segment label;
};

jta_result jta_load_materials(const jio_memory_file* mem_file, u32* n_mat, jta_material** pp_materials);

#endif //JTA_JTAMATERIALS_H
