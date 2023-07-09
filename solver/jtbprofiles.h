//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBPROFILES_H
#define JTB_JTBPROFILES_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtberr.h"

typedef struct jtb_profile_list_struct jtb_profile_list;
struct jtb_profile_list_struct
{
    uint32_t count;
    jio_string_segment* labels;
    f32* area;
    f32* second_moment_of_area;
    f32* equivalent_radius;
    f32 min_equivalent_radius;
    f32 max_equivalent_radius;
};

jtb_result jtb_load_profiles(const jio_memory_file* mem_file, jtb_profile_list* profile_list);

#endif //JTB_JTBPROFILES_H
