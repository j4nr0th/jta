//
// Created by jan on 5.7.2023.
//

#include "jtaerr.h"

static const char* const code_strings[JTA_RESULT_COUNT] =
        {
            [JTA_RESULT_SUCCESS] = "Success",
            [JTA_RESULT_BAD_ALLOC] = "Memory allocation failed",
            [JTA_RESULT_BAD_IO] = "IO operation failed",
            [JTA_RESULT_BAD_INPUT] = "Input file was invalid",
            [JTA_RESULT_BAD_NUM_BC] = "Invalid boundary conditions were specified",
        };

const char* jta_result_to_str(jta_result res)
{
    if (res < JTA_RESULT_COUNT && res >= JTA_RESULT_SUCCESS)
    {
        return code_strings[res];
    }
    return "Unknown";
}
