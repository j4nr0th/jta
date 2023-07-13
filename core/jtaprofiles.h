//
// Created by jan on 5.7.2023.
//

#ifndef JTA_JTAPROFILES_H
#define JTA_JTAPROFILES_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtaerr.h"

typedef struct jta_profile_list_struct jta_profile_list;
struct jta_profile_list_struct
{
    uint32_t count;
    jio_string_segment* labels;
    f32* area;
    f32* second_moment_of_area;
    f32* equivalent_radius;
    f32 min_equivalent_radius;
    f32 max_equivalent_radius;
};

jta_result jta_load_profiles(const jio_memory_file* mem_file, jta_profile_list* profile_list);

#endif //JTA_JTAPROFILES_H
