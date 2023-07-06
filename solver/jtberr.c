//
// Created by jan on 5.7.2023.
//

#include "jtberr.h"

static const char* const code_strings[JTB_RESULT_COUNT] =
        {
            [JTB_RESULT_SUCCESS] = "Success",
            [JTB_RESULT_BAD_ALLOC] = "Memory allocation failed",
            [JTB_RESULT_BAD_IO] = "IO operation failed",
            [JTB_RESULT_BAD_INPUT] = "Input file was invalid",
        };

const char* jtb_result_to_str(jtb_result res)
{
    if (res < JTB_RESULT_COUNT && res >= JTB_RESULT_SUCCESS)
    {
        return code_strings[res];
    }
    return "Unknown";
}
