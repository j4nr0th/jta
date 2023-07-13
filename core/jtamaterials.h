//
// Created by jan on 5.7.2023.
//

#ifndef JTA_JTAMATERIALS_H
#define JTA_JTAMATERIALS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtaerr.h"

typedef struct jta_material_list_struct jta_material_list;
struct jta_material_list_struct
{
    u32 count;
    f32* elastic_modulus;
    f32* density;
    f32* tensile_strength;
    f32* compressive_strength;
    jio_string_segment* labels;
};

jta_result jta_load_materials(const jio_memory_file* mem_file, jta_material_list* material_list);

#endif //JTA_JTAMATERIALS_H
