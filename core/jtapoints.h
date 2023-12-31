//
// Created by jan on 5.7.2023.
//

#ifndef JTA_JTAPOINTS_H
#define JTA_JTAPOINTS_H

#include <jio/iocsv.h>
#include "../common/common.h"
#include "jtaerr.h"

typedef struct jta_point_list_T jta_point_list;
struct jta_point_list_T
{
    uint32_t count;
    f32* p_x;
    f32* p_y;
    f32* p_z;
    f32* max_radius;
    jio_string_segment* label;
};

jta_result jta_load_points(const jio_context* io_ctx, const jio_memory_file* mem_file, jta_point_list* p_list);

void jta_free_points(jta_point_list* list);

#endif //JTA_JTAPOINTS_H
