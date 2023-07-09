//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBELEMENTS_H
#define JTB_JTBELEMENTS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtberr.h"
#include "jtbpoints.h"
#include "jtbmaterials.h"
#include "jtbprofiles.h"


typedef struct jtb_element_list_struct jtb_element_list;
struct jtb_element_list_struct
{
    uint32_t count;
    uint32_t* i_point0;
    uint32_t* i_point1;
    uint32_t* i_material;
    uint32_t* i_profile;
    jio_string_segment* labels;
    f32* lengths;
    f32 min_len;
    f32 max_len;
};


jtb_result jtb_load_elements(
        const jio_memory_file* mem_file, const jtb_point_list* points, u32 n_mat, const jtb_material* materials,
        const jtb_profile_list* profiles, jtb_element_list* element_list);

#endif //JTB_JTBELEMENTS_H
