//
// Created by jan on 30.6.2023.
//

#include <jdm.h>
#include "iocsv.h"

//TODO: remove this when done testing
#define TEST_MSG(msg, ...) printf(msg __VA_OPT__(,) __VA_ARGS__)

static inline jio_string_segment extract_string_segment(const char* ptr, const char** p_end, const char separator, bool trim_whitespace)
{
    const char* end = *p_end;
    const char* real_end = strchr(ptr, separator);
    if (real_end == NULL)
    {
        *p_end = NULL;
        return (jio_string_segment){0};
    }
    if (end > real_end)
    {
        end = real_end;
    }
    if (end == ptr)
    {
        *p_end = ptr + 1;
        return (jio_string_segment){.begin = ptr, .len = 0};
    }
    assert(end > ptr);
    //  Trim the string segment
    if (trim_whitespace)
    {
        while (iswhitespace(*ptr) && ptr != end) ++ptr;
        while (iswhitespace(*(end - 1) && ptr != end)) --end;
    }

    *p_end = end;
    return (jio_string_segment){.begin = ptr, .len = end - ptr};
}

jio_result jio_parse_csv(
        const jio_memory_file* mem_file, char separator, bool trim_whitespace, bool has_headers,
        jio_csv_data** pp_csv, jallocator* allocator, linear_jallocator* lin_allocator)
{
    JDM_ENTER_FUNCTION;
    jio_result res;
    void* const base = lin_jalloc_get_current(lin_allocator);
    jio_csv_data* csv = NULL;
    if (!pp_csv || !allocator || !lin_allocator)
    {
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }
    csv = jalloc(allocator, sizeof(*csv));
    if (!csv)
    {
        res = JIO_RESULT_BAD_ALLOC;
        goto end;
    }

    memset(csv, 0, sizeof(*csv));
    csv->allocator = allocator;

    //  Parse the first row
    const char* row_begin = mem_file->ptr;
    const char* row_end = strchr(row_begin, '\n');

    //  First row may have special treatment if it contains the headers
    size_t column_count = 0;
    jio_string_segment segment;
    const char* end = row_end, *begin = row_begin;
process_row:
    end = row_end;
    segment = extract_string_segment(begin, &end, separator, trim_whitespace);
    if (*end && *end != '\n')
    {
        TEST_MSG("Column %zu is %.*s\n", column_count, (int)segment.len, segment.begin);
        column_count += 1;
        begin = end + 1;
        goto process_row;
    }
    TEST_MSG("Found %zu columns\n", column_count);

    if (has_headers)
    {

    }
    else
    {

    }


    *pp_csv = csv;
    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
end:
    lin_jalloc_set_current(lin_allocator, base);
    if (allocator)
    {
        jfree(allocator, csv);
    }
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_release(jio_csv_data* data)
{
    JDM_ENTER_FUNCTION;

    jfree(data->allocator, data);

    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
}
