//
// Created by jan on 9.7.2023.
//

#ifndef JTB_JTBNATURALBCS_H
#define JTB_JTBNATURALBCS_H
#include "jtberr.h"
#include "jtbpoints.h"

typedef struct jtb_natural_boundary_condition_list_struct jtb_natural_boundary_condition_list;
struct jtb_natural_boundary_condition_list_struct
{
    uint32_t count;
    uint32_t* i_point;
    f32* x;
    f32* y;
    f32* z;
    f32 max_mag;
    f32 min_mag;
};



jtb_result jtb_load_natural_boundary_conditions(const jio_memory_file* mem_file, const jtb_point_list* point_list, jtb_natural_boundary_condition_list* bcs);

#endif //JTB_JTBNATURALBCS_H
