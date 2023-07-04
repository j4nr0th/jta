//
// Created by jan on 30.6.2023.
//

#ifndef JTB_IOCSV_H
#define JTB_IOCSV_H
#include "ioerr.h"
#include "iobase.h"
#include <stdint.h>
#include <stddef.h>

typedef struct jio_csv_column_struct jio_csv_column;
struct jio_csv_column_struct
{
    jio_string_segment header;      //  Optional header
    uint32_t count;                   //  How many elements are used
    uint32_t capacity;                //  How many elements there is space for
    jio_string_segment* elements;   //  Element array
};
typedef struct jio_csv_data_struct jio_csv_data;

jio_result jio_csv_column_index(const jio_csv_data* data, const jio_csv_column* column, uint32_t* p_idx);

jio_result jio_parse_csv(
        const jio_memory_file* mem_file, const char* restrict separator, bool trim_whitespace, bool has_headers,
        jio_csv_data** pp_csv, jallocator* allocator, linear_jallocator* lin_allocator);

jio_result jio_process_csv_exact(
        const jio_memory_file* mem_file, const char* separator, uint32_t column_count, const jio_string_segment* headers,
        bool (** converter_array)(jio_string_segment*, void*), void** param_array, linear_jallocator* lin_allocator);

jio_result jio_csv_get_column(const jio_csv_data* data, uint32_t index, const jio_csv_column** pp_column);

jio_result jio_csv_get_column_by_name(const jio_csv_data* data, const char* name, const jio_csv_column** pp_column);

jio_result jio_csv_get_column_by_name_segment(const jio_csv_data* data, const jio_string_segment* name, const jio_csv_column** pp_column);

jio_result jio_csv_add_rows(jio_csv_data* data, uint32_t position, uint32_t row_count, const jio_string_segment* const* rows);

jio_result jio_csv_add_cols(jio_csv_data* data, uint32_t position, uint32_t col_count, const jio_csv_column* cols);

jio_result jio_csv_remove_rows(jio_csv_data* data, uint32_t position, uint32_t row_count);

jio_result jio_csv_remove_cols(jio_csv_data* data, uint32_t position, uint32_t col_count);

jio_result jio_csv_replace_rows(jio_csv_data* data, uint32_t begin, uint32_t count, uint32_t row_count, const jio_string_segment* const* rows);

jio_result jio_csv_replace_cols(jio_csv_data* data, uint32_t begin, uint32_t count, uint32_t col_count, const jio_csv_column* cols);

jio_result jio_csv_shape(const jio_csv_data* data, uint32_t* p_rows, uint32_t* p_cols);

jio_result jio_csv_release(jio_csv_data* data);

jio_result jio_csv_print_size(const jio_csv_data* data, size_t* p_size, size_t separator_length, uint32_t extra_padding, bool same_width);

jio_result jio_csv_print(const jio_csv_data* data, size_t* p_usage, char* restrict buffer, const char* separator, uint32_t extra_padding, bool same_width, bool align_left);


#endif //JTB_IOCSV_H
