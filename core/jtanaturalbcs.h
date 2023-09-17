//
// Created by jan on 9.7.2023.
//

#ifndef JTA_JTANATURALBCS_H
#define JTA_JTANATURALBCS_H
#include "jtaerr.h"
#include "jtapoints.h"

typedef struct jta_natural_boundary_condition_list_T jta_natural_boundary_condition_list;
struct jta_natural_boundary_condition_list_T
{
    uint32_t count;
    uint32_t* i_point;
    f32* x;
    f32* y;
    f32* z;
    f32 max_mag;
    f32 min_mag;
};



jta_result jta_load_natural_boundary_conditions(
        const jio_context* io_ctx, const jio_memory_file* mem_file, const jta_point_list* point_list,
        jta_natural_boundary_condition_list* bcs);

void jta_free_natural_boundary_conditions(jta_natural_boundary_condition_list* bcs);

#endif //JTA_JTANATURALBCS_H
