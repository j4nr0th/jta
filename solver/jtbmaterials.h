//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBMATERIALS_H
#define JTB_JTBMATERIALS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtberr.h"



typedef struct jtb_material_struct jtb_material;
struct jtb_material_struct
{
    f64 density;
    f64 elastic_modulus;
    f64 tensile_strength;
    f64 compressive_strength;
    jio_string_segment label;
};

jtb_result jtb_load_materials(const jio_memory_file* mem_file, u32* n_mat, jtb_material** pp_materials);

#endif //JTB_JTBMATERIALS_H
