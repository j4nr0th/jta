//
// Created by jan on 30.6.2023.
//

#include "ioerr.h"

static const char* const jio_result_strings[JIO_RESULT_COUNT] =
        {
                [JIO_RESULT_SUCCESS] = "Success",
                [JIO_RESULT_BAD_PATH] = "Bad file path",
                [JIO_RESULT_BAD_MAP] = "Failed mapping file to memory",
                [JIO_RESULT_NULL_ARG] = "An argument to the function was null",
                [JIO_RESULT_BAD_ALLOC] = "Allocation failed",
        };

const char* jio_result_to_str(jio_result res)
{
    if (res < JIO_RESULT_COUNT)
        return jio_result_strings[res];
    return "Unknown";
}
