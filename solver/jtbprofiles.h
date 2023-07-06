//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBPROFILES_H
#define JTB_JTBPROFILES_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtberr.h"


typedef struct jtb_profile_struct jtb_profile;
struct jtb_profile_struct
{
    f64 area;
    f64 second_moment_of_area;
    jio_string_segment label;
};

jtb_result jtb_load_profiles(const jio_memory_file* mem_file, u32* n_pro, jtb_profile** pp_profiles);

#endif //JTB_JTBPROFILES_H
