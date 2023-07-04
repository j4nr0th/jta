//
// Created by jan on 1.7.2023.
//
#include "../iocsv.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#ifndef NDEBUG
#define DBG_STOP __builtin_trap()
#else
#define DGB_STOP (void)0;
#endif
#define ASSERT(x) if (!(x)) {fputs("Failed assertion \"" #x "\"\n", stderr); DBG_STOP; exit(EXIT_FAILURE);} (void)0


int error_hook_fn(const char* thread_name, uint32_t stack_trace_count, const char*const* stack_trace, jdm_message_level level, uint32_t line, const char* file, const char* function, const char* message, void* param)
{
    const char* err_level_str;
    FILE* f_out;
    switch (level)
    {
    default:
        err_level_str = "unknown";
        f_out = stderr;
        break;
    case JDM_MESSAGE_LEVEL_CRIT:
        err_level_str = "critical error";
        f_out = stderr;
        break;
    case JDM_MESSAGE_LEVEL_ERR:
        err_level_str = "error";
        f_out = stderr;
        break;
    case JDM_MESSAGE_LEVEL_WARN:
        err_level_str = "warning";
        f_out = stderr;
        break;
    case JDM_MESSAGE_LEVEL_TRACE:
        err_level_str = "trace";
        f_out = stdout;
        break;
    case JDM_MESSAGE_LEVEL_DEBUG:
        err_level_str = "debug";
        f_out = stdout;
        break;
    case JDM_MESSAGE_LEVEL_INFO:
        err_level_str = "info";
        f_out = stdout;
        break;
    }
    fprintf(f_out, "%s from \"%s\" at line %u in file \"%s\", message: %s\n", err_level_str, function, line, file, message);
    return 0;
}

static inline void print_csv(const jio_csv_data* data)
{
    jio_result res;
    uint32_t rows, cols;
    res = jio_csv_shape(data, &rows, &cols);
    ASSERT(res == JIO_RESULT_SUCCESS);
    for (uint32_t i = 0; i < cols; ++i)
    {
        const jio_csv_column* p_column;
        res = jio_csv_get_column(data, i, &p_column);
        ASSERT(res == JIO_RESULT_SUCCESS);
        printf("Column %u has a header \"%.*s\" and length of %u:\n", i, (int)p_column->header.len, p_column->header.begin, p_column->count);
        for (uint32_t j = 0; j < rows; ++j)
        {
            jio_string_segment* segment = p_column->elements + j;
            printf("\telement %u: \"%.*s\"\n", j, (int)segment->len, segment->begin);
        }
    }
}

int main()
{
    jallocator* allocator = jallocator_create((1 << 20), 1);
    ASSERT(allocator);
    jdm_init_thread("master", JDM_MESSAGE_LEVEL_NONE, 32, 32, allocator);
    jdm_set_hook(error_hook_fn, NULL);
    linear_jallocator* lin_allocator = lin_jallocator_create(1 << 20);
    ASSERT(lin_allocator);
    void* const base = lin_jalloc_get_current(lin_allocator);

    jio_memory_file csv_file;
    jio_result res = jio_memory_file_create("../csv_test_simple.csv", &csv_file, 0, 0, 0);
    ASSERT(res == JIO_RESULT_SUCCESS);

    jio_csv_data* csv_data;
    //  Test failures
    res = jio_parse_csv(NULL, ',', true, true, &csv_data, allocator, lin_allocator);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_parse_csv(&csv_file, ',', true, true, NULL, allocator, lin_allocator);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_parse_csv(&csv_file, ',', true, true, &csv_data, NULL, lin_allocator);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_parse_csv(&csv_file, ',', true, true, &csv_data, allocator, NULL);
    ASSERT(res == JIO_RESULT_NULL_ARG);

    //  Test correct version
    res = jio_parse_csv(&csv_file, ',', true, true, &csv_data, allocator, lin_allocator);
    ASSERT(res == JIO_RESULT_SUCCESS);
    printf("\nCsv contents before editing:\n\n");
    print_csv(csv_data);

    jio_csv_column extra_column =
            {
            .header = {.begin = "extra:)", .len = sizeof("extra:)") - 1},
            .count = 4,
            .capacity = 4,
            .elements = jalloc(allocator, sizeof(*extra_column.elements) * 4),
            };
    ASSERT(extra_column.elements != NULL);
    extra_column.elements[0].begin = "Ass", extra_column.elements[0].len = 3;
    extra_column.elements[1].begin = "We", extra_column.elements[1].len = 2;
    extra_column.elements[2].begin = "Can", extra_column.elements[2].len = 3;
    extra_column.elements[3].begin = "!", extra_column.elements[3].len = 1;

    //  Test incorrect values
    //      No csv file
    res = jio_csv_add_cols(NULL, 2, 1, &extra_column);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    //      Column position out of bounds
    res = jio_csv_add_cols(csv_data, 9201412, 1, &extra_column);
    ASSERT(res == JIO_RESULT_BAD_INDEX);
    //      Columns not provided
    res = jio_csv_add_cols(csv_data, 2, 1, NULL);
    ASSERT(res == JIO_RESULT_NULL_ARG);


    //  Test correct version
    res = jio_csv_add_cols(csv_data, 2, 1, &extra_column);
    ASSERT(res == JIO_RESULT_SUCCESS);

    printf("\nCsv contents after adding column:\n\n");
    print_csv(csv_data);

    //  Test incorrect version
    //      No csv file
    res = jio_csv_remove_cols(NULL, 0, 2);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    //      Position out of bounds
    res = jio_csv_remove_cols(csv_data, 1212312312, 2);
    ASSERT(res == JIO_RESULT_BAD_INDEX);

    //  Test correct version
    res = jio_csv_remove_cols(csv_data, 0, 2);
    ASSERT(res == JIO_RESULT_SUCCESS);
    printf("Csv contents after removing columns:\n");
    print_csv(csv_data);

    jio_string_segment** row_array = lin_jalloc(lin_allocator, sizeof(jio_string_segment*) * 10);
    ASSERT(row_array);
    for (uint32_t i = 0; i < 10; ++i)
    {
        row_array[i] = lin_jalloc(lin_allocator, sizeof(*row_array[i]) * 4);
        ASSERT(row_array[i]);
        for (uint32_t j = 0; j < 4; ++j)
        {
            row_array[i][j] = (jio_string_segment){.begin = "Funni", .len = 5};
        }
    }

    //  Test the incorrect versions
    //      No csv file
    res = jio_csv_add_rows(NULL, 1, 10, (const jio_string_segment**) row_array);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    //      Position out of bounds
    res = jio_csv_add_rows(csv_data, 112412412, 10, (const jio_string_segment**) row_array);
    ASSERT(res == JIO_RESULT_BAD_INDEX);
    //      No row array
    res = jio_csv_add_rows(csv_data, 1, 10, (const jio_string_segment**) NULL);
    ASSERT(res == JIO_RESULT_NULL_ARG);

    //  Test getting the same column by both index, name, and string segment
    const char* extra_name = "extra:)";
    const jio_csv_column*  p_column1, *p_column2, *p_column3;

    res = jio_csv_get_column(csv_data, 0, &p_column1);
    ASSERT(res == JIO_RESULT_SUCCESS);

    res = jio_csv_get_column_by_name(csv_data, extra_name, &p_column2);
    ASSERT(res == JIO_RESULT_SUCCESS);
    //  Test the incorrect versions
    res = jio_csv_get_column_by_name(NULL, extra_name, &p_column2);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_get_column_by_name(csv_data, extra_name, NULL);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_get_column_by_name(csv_data, "gamer god", &p_column2);
    ASSERT(res == JIO_RESULT_BAD_CSV_HEADER);
    res = jio_csv_get_column_by_name(csv_data, NULL, &p_column2);
    ASSERT(res == JIO_RESULT_NULL_ARG);

    const jio_string_segment ss = (jio_string_segment){ .begin = extra_name, .len = 7 };
    res = jio_csv_get_column_by_name_segment(csv_data, &ss, &p_column3);
    ASSERT(res == JIO_RESULT_SUCCESS);
    //  Test the incorrect versions
    res = jio_csv_get_column_by_name_segment(NULL, &ss, &p_column2);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_get_column_by_name_segment(csv_data, &ss, NULL);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_get_column_by_name_segment(csv_data, NULL, &p_column2);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    const jio_string_segment bad_ss = (jio_string_segment){ .begin = extra_name, .len = 6 };
    res = jio_csv_get_column_by_name_segment(csv_data, &bad_ss, &p_column2);
    ASSERT(res == JIO_RESULT_BAD_CSV_HEADER);


    ASSERT(p_column1 == p_column2);
    ASSERT(p_column2 == p_column3);



    //  Test the correct version
    res = jio_csv_add_rows(csv_data, 1, 10, (const jio_string_segment**) row_array);
    ASSERT(res == JIO_RESULT_SUCCESS);
    printf("\nCsv contents after adding rows:\n\n");
    print_csv(csv_data);

    //  Test the incorrect versions
    //      No csv file
    res = jio_csv_remove_rows(NULL, 3, 5);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_remove_rows(csv_data, 212412512, 5);
    ASSERT(res == JIO_RESULT_BAD_INDEX);

    //  Test the correct version
    res = jio_csv_remove_rows(csv_data, 3, 5);
    ASSERT(res == JIO_RESULT_SUCCESS);
    printf("Csv contents after removing rows:\n");
    print_csv(csv_data);


    //  Test replacing rows
    for (uint32_t i = 0; i < 10; ++i)
    {
        for (uint32_t j = 0; j < 4; ++j)
        {
            row_array[i][j] = (jio_string_segment){.begin = "Not funne", .len = 9};
        }
    }

    //  Test the incorrect version
    res = jio_csv_replace_rows(NULL, 3, 4, 6, (const jio_string_segment* const*) row_array);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_replace_rows(csv_data, 13, 4, 6, (const jio_string_segment* const*) row_array);
    ASSERT(res == JIO_RESULT_BAD_INDEX);
    res = jio_csv_replace_rows(csv_data, 3, 27123, 6, (const jio_string_segment* const*) row_array);
    ASSERT(res == JIO_RESULT_BAD_INDEX);
    res = jio_csv_replace_rows(csv_data, 3, 4, 6, NULL);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    //  Test the correct version
    res = jio_csv_replace_rows(csv_data, 3, 4, 6, (const jio_string_segment* const*) row_array);
    ASSERT(res == JIO_RESULT_SUCCESS);
    printf("Csv contents after replacing rows:\n");
    print_csv(csv_data);

    size_t data_size = 0;
    //  Incorrect versions
    res = jio_csv_print_size(NULL, &data_size, 1, 1, true);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    res = jio_csv_print_size(csv_data, NULL, 1, 1, true);
    ASSERT(res == JIO_RESULT_NULL_ARG);
    //  Correct version
    res = jio_csv_print_size(csv_data, &data_size, 1, 1, true);
    ASSERT(res == JIO_RESULT_SUCCESS);
    printf("Space needed to print the data: %zu bytes\n", data_size);

    jio_memory_file f_out;
    res = jio_memory_file_create("out.csv", &f_out, 1, 1, data_size);
    ASSERT(res == JIO_RESULT_SUCCESS);
    ASSERT(f_out.file_size >= data_size);
    ASSERT(f_out.can_write == 1);

    size_t real_usage = 0;
    res = jio_csv_print(csv_data, &real_usage, f_out.ptr, ",", 1, true, false);
    ASSERT(res == JIO_RESULT_SUCCESS);

    res = jio_memory_file_sync(&f_out, 1);
    ASSERT(res == JIO_RESULT_SUCCESS);
    jio_memory_file_destroy(&f_out);



    for (uint32_t i = 0; i < 10; ++i)
    {
        lin_jfree(lin_allocator, row_array[i]);
    }
    lin_jfree(lin_allocator, row_array);

    jio_csv_release(csv_data);
    jio_memory_file_destroy(&csv_file);

    jdm_cleanup_thread();
    int_fast32_t i_pool, i_block;
    ASSERT(jallocator_verify(allocator, &i_pool, &i_block) == 0);
    uint_fast32_t leaked_array[128];
    uint_fast32_t count_leaked = jallocator_count_used_blocks(allocator, 128, leaked_array);
    for (uint_fast32_t i = 0; i < count_leaked; ++i)
    {
        fprintf(stderr, "Leaked block %"PRIuFAST32"\n", leaked_array[i]);
    }
    ASSERT(count_leaked == 0);
    ASSERT(base == lin_jalloc_get_current(lin_allocator));
    lin_jallocator_destroy(lin_allocator);

    return 0;
}
