//
// Created by jan on 5.7.2023.
//

#ifndef JTA_JTAERR_H
#define JTA_JTAERR_H
#include "../../common/common.h"

typedef enum jta_result_enum jta_result;
enum jta_result_enum
{
    JTA_RESULT_SUCCESS,

    JTA_RESULT_BAD_ALLOC,
    JTA_RESULT_BAD_IO,
    JTA_RESULT_BAD_INPUT,
    JTA_RESULT_BAD_NUM_BC,
    JTA_RESULT_BAD_CFG_ENTRY,

    JTA_RESULT_COUNT,
};

const char* jta_result_to_str(jta_result res);

#endif //JTA_JTAERR_H
