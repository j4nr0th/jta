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


typedef struct jtb_element_struct jtb_element;
struct jtb_element_struct
{
    u32 i_point0;
    u32 i_point1;
    u32 i_material;
    u32 i_profile;
    jio_string_segment label;
};


jtb_result jtb_load_elements(const jio_memory_file* mem_file, u32 n_pts, const jtb_point* points, u32 n_mat, const jtb_material* materials, u32 n_pro, const jtb_profile* profiles, u32* n_elm, jtb_element** pp_elements);

#endif //JTB_JTBELEMENTS_H
