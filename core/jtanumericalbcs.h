//
// Created by jan on 10.7.2023.
//

#ifndef JTA_JTANUMERICALBCS_H
#define JTA_JTANUMERICALBCS_H
#include "jtaerr.h"
#include "jtapoints.h"

enum jta_numerical_boundary_condition_type_enum : uint8_t
{
    JTA_NUMERICAL_BC_TYPE_X = 1 << 0,
    JTA_NUMERICAL_BC_TYPE_Y = 1 << 1,
    JTA_NUMERICAL_BC_TYPE_Z = 1 << 2,
};
typedef enum jta_numerical_boundary_condition_type_enum jta_numerical_boundary_condition_type;

typedef struct jta_numerical_boundary_condition_list_T jta_numerical_boundary_condition_list;
struct jta_numerical_boundary_condition_list_T
{
    uint32_t count;
    uint32_t* i_point;
    f32* x;
    f32* y;
    f32* z;
    jta_numerical_boundary_condition_type* type;
};

jta_result jta_load_numerical_boundary_conditions(
        const jio_context* io_ctx, const jio_memory_file* mem_file, const jta_point_list* point_list,
        jta_numerical_boundary_condition_list* bcs);

void jta_free_numerical_boundary_conditions(jta_numerical_boundary_condition_list* bcs);

#endif //JTA_JTANUMERICALBCS_H
