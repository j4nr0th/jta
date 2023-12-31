//
// Created by jan on 5.7.2023.
//

#ifndef JTA_JTAELEMENTS_H
#define JTA_JTAELEMENTS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtaerr.h"
#include "jtapoints.h"
#include "jtamaterials.h"
#include "jtaprofiles.h"


typedef struct jta_element_list_T jta_element_list;
struct jta_element_list_T
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


jta_result jta_load_elements(
        const jio_context* io_ctx, const jio_memory_file* mem_file, const jta_point_list* points,
        const jta_material_list* materials, const jta_profile_list* profiles, jta_element_list* element_list);

void jta_free_elements(jta_element_list* element_list);

#endif //JTA_JTAELEMENTS_H
