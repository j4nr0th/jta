//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBPOINTS_H
#define JTB_JTBPOINTS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtberr.h"

typedef struct jtb_point_list_struct jtb_point_list;
struct jtb_point_list_struct
{
    uint32_t count;
    f32* p_x;
    f32* p_y;
    f32* p_z;
    f32* max_radius;
    jio_string_segment* label;
};

jtb_result jtb_load_points(const jio_memory_file* mem_file, jtb_point_list* p_list);

#endif //JTB_JTBPOINTS_H
