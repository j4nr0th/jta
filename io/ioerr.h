//
// Created by jan on 30.6.2023.
//

#ifndef JTB_IOERR_H
#define JTB_IOERR_H


enum jio_result_enum : unsigned
{
    JIO_RESULT_SUCCESS = 0,
    JIO_RESULT_BAD_PATH,
    JIO_RESULT_BAD_MAP,

    JIO_RESULT_BAD_ALLOC,
    JIO_RESULT_NULL_ARG,

    JIO_RESULT_COUNT,
};
typedef enum jio_result_enum jio_result;

const char* jio_result_to_str(jio_result res);


#endif //JTB_IOERR_H
