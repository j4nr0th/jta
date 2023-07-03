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
    size_t count;                   //  How many elements are used
    size_t capacity;                //  How many elements there is space for
    jio_string_segment* elements;   //  Element array
};

typedef struct jio_csv_data_struct jio_csv_data;
struct jio_csv_data_struct
{
    jallocator* allocator;                  //  Allocator used by this csv
    uint32_t column_capacity;               //  Max size of columns before resizing the array
    uint32_t column_count;                  //  Number of columns used
    uint32_t column_length;                 //  Length of each column
    jio_csv_column* columns;                //  Columns themselves
};


jio_result jio_parse_csv(
        const jio_memory_file* mem_file, char separator, bool trim_whitespace, bool has_headers,
        jio_csv_data** pp_csv, jallocator* allocator, linear_jallocator* lin_allocator);

jio_result jio_csv_get_column(const jio_csv_data* data, uint32_t index, const jio_csv_column** pp_column);

jio_result jio_csv_get_column_by_name(const jio_csv_data* data, const char* name, const jio_csv_column** pp_column);

jio_result jio_csv_get_column_by_name_segment(const jio_csv_data* data, const jio_string_segment* name, const jio_csv_column** p_column);

jio_result jio_csv_transpose(const jio_csv_data* data);

jio_result jio_csv_add_rows(jio_csv_data* data, uint32_t position, uint32_t row_count, const jio_string_segment* const* rows);

jio_result jio_csv_add_cols(jio_csv_data* data, uint32_t position, uint32_t col_position, const jio_csv_column* cols);

jio_result jio_csv_remove_rows(jio_csv_data* data, uint32_t position, uint32_t row_count);

jio_result jio_csv_remove_cols(jio_csv_data* data, uint32_t position, uint32_t col_count);

jio_result jio_csv_replace_rows(jio_csv_data* data, uint32_t position, uint32_t row_count, const jio_string_segment* const* rows);

jio_result jio_csv_replace_cols(jio_csv_data* data, uint32_t position, uint32_t col_position, const jio_csv_column* cols);

jio_result jio_csv_shape(const jio_csv_data* data, uint32_t* p_rows, uint32_t* p_cols);

jio_result jio_csv_release(jio_csv_data* data);


#endif //JTB_IOCSV_H
