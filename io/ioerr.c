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
                [JIO_RESULT_BAD_CSV_FORMAT] = "Csv file was formatted badly",
                [JIO_RESULT_BAD_INDEX] = "Requested index was invalid",
                [JIO_RESULT_BAD_CSV_HEADER] = "Requested csv name does not match any headers",
                [JIO_RESULT_BAD_CSV_COLUMN] = "Csv column has invalid shape",
                [JIO_RESULT_BAD_PTR] = "Pointer given as argument was invalid",
                [JIO_RESULT_BAD_CONVERTER] = "Converter given was not provided",
                [JIO_RESULT_BAD_VALUE] = "Value in csv file was invalid",
                [JIO_RESULT_BAD_ACCESS] = "File's access mode does not allow for operation",
        };

const char* jio_result_to_str(jio_result res)
{
    if (res < JIO_RESULT_COUNT)
        return jio_result_strings[res];
    return "Unknown";
}
