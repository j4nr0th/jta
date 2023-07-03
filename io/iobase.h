//
// Created by jan on 30.6.2023.
//

#ifndef JTB_IOBASE_H
#define JTB_IOBASE_H

#include <limits.h>
#include <jdm.h>
#include <jmem/jmem.h>
#include "../common/common.h"
#include "ioerr.h"

typedef struct jio_memory_file_struct jio_memory_file;

struct jio_memory_file_struct
{
    void* ptr;
    u64 file_size;
#ifndef _WIN32
    char name[PATH_MAX];
#else
    char name[4096];
#endif
};

typedef struct jio_string_segment_struct jio_string_segment;
struct jio_string_segment_struct
{
    const char* begin;
    size_t len;
};

bool iswhitespace(char32_t c);

bool string_segment_cmp(const jio_string_segment* first, const jio_string_segment* second);

bool string_segment_cmp_case(const jio_string_segment* first, const jio_string_segment* second);

bool string_segment_cmp_str(const jio_string_segment* first, const char* str);

bool string_segment_cmp_str_case(const jio_string_segment* first, const char* str);

jio_result jio_map_file_to_memory(const char* filename, jio_memory_file* p_file_out);

void jio_unmap_file(jio_memory_file* p_file_out);

bool jio_check_if_file_is_same(const jio_memory_file* f1, const char* filename);



#endif //JTB_IOBASE_H
