//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBPOINTS_H
#define JTB_JTBPOINTS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtberr.h"

typedef struct jtb_point_struct jtb_point;
struct jtb_point_struct
{
    f64 x;
    f64 y;
    f64 z;
    jio_string_segment label;
};

jtb_result jtb_load_points(const jio_memory_file* mem_file, u32* n_pts, jtb_point** pp_points);

#endif //JTB_JTBPOINTS_H
