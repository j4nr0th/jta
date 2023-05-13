//
// Created by jan on 13.5.2023.
//

#ifndef JTB_FORMATTED_H
#define JTB_FORMATTED_H
#include "../mem/lin_alloc.h"
#include <string.h>

char* lin_sprintf(linear_allocator allocator, size_t* p_size, const char* fmt, ...);


#endif //JTB_FORMATTED_H
