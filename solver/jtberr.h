//
// Created by jan on 5.7.2023.
//

#ifndef JTB_JTBERR_H
#define JTB_JTBERR_H
#include "../../common/common.h"

typedef enum jtb_result_enum jtb_result;
enum jtb_result_enum
{
    JTB_RESULT_SUCCESS,

    JTB_RESULT_BAD_ALLOC,
    JTB_RESULT_BAD_IO,
    JTB_RESULT_BAD_INPUT,

    JTB_RESULT_COUNT,
};

const char* jtb_result_to_str(jtb_result res);

#endif //JTB_JTBERR_H
